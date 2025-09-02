// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <chrono>
#include <atomic>
#include <mutex>
#include <functional>
#include <string>

/**
 * @file circuit_breaker.hpp
 * @brief Circuit breaker pattern for API reliability and fault tolerance
 * 
 * Implements the circuit breaker pattern to provide resilience against
 * API failures, rate limits, and service degradation by automatically
 * failing fast when the LLM service is unavailable.
 */

namespace cql {
namespace meta_prompt {

/**
 * @brief States of the circuit breaker
 */
enum class CircuitState {
    CLOSED,      ///< Normal operation - requests pass through
    OPEN,        ///< Failing fast - requests are rejected immediately  
    HALF_OPEN    ///< Testing recovery - limited requests allowed
};

/**
 * @brief Configuration for circuit breaker behavior
 */
struct CircuitBreakerConfig {
    size_t failure_threshold = 5;                           ///< Consecutive failures before opening
    std::chrono::seconds timeout_duration{60};              ///< Time to wait before testing recovery
    size_t success_threshold = 3;                          ///< Successes needed to close from half-open
    std::chrono::seconds rolling_window{300};               ///< Rolling window for failure rate calculation
    double failure_rate_threshold = 0.5;                    ///< Failure rate to trigger opening (0.0-1.0)
    size_t minimum_requests = 10;                          ///< Minimum requests before rate-based opening
};

/**
 * @brief Statistics for circuit breaker monitoring
 */
struct CircuitBreakerStats {
    CircuitState current_state = CircuitState::CLOSED;
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t failed_requests = 0;
    size_t rejected_requests = 0;
    std::chrono::system_clock::time_point last_failure_time;
    std::chrono::system_clock::time_point last_success_time;
    std::chrono::system_clock::time_point state_changed_time;
    double current_failure_rate = 0.0;
};

/**
 * @brief Circuit breaker for protecting against API failures
 * 
 * CircuitBreaker implements the circuit breaker pattern to provide
 * fault tolerance for LLM API calls. It monitors request success/failure
 * patterns and automatically fails fast when the service appears unavailable,
 * allowing the system to gracefully degrade to local compilation.
 * 
 * The circuit breaker operates in three states:
 * - CLOSED: Normal operation, all requests pass through
 * - OPEN: Service appears down, requests fail immediately
 * - HALF_OPEN: Testing if service has recovered
 */
class CircuitBreaker {
public:
    /**
     * @brief Constructor with configuration
     * 
     * @param config CircuitBreakerConfig settings
     */
    explicit CircuitBreaker(const CircuitBreakerConfig& config = {});

    /**
     * @brief Destructor
     */
    ~CircuitBreaker();

    /**
     * @brief Execute a function with circuit breaker protection
     * 
     * Attempts to execute the provided function if the circuit is closed
     * or half-open. Records the result and updates circuit state accordingly.
     * 
     * @tparam Func Function type returning bool (success/failure)
     * @param func Function to execute - should return true on success
     * @param operation_name Name for logging purposes
     * @return true if operation succeeded, false if failed or circuit open
     */
    template<typename Func>
    [[nodiscard]] bool execute(Func&& func, const std::string& operation_name = "operation");

    /**
     * @brief Check if requests can pass through the circuit
     * 
     * @return true if circuit allows requests (CLOSED or HALF_OPEN states)
     */
    [[nodiscard]] bool can_execute() const;

    /**
     * @brief Manually record a successful operation
     * 
     * Use when operation success/failure is determined externally.
     */
    void record_success();

    /**
     * @brief Manually record a failed operation
     * 
     * Use when operation success/failure is determined externally.
     * 
     * @param error_message Optional error description for logging
     */
    void record_failure(const std::string& error_message = "");

    /**
     * @brief Force the circuit to open state
     * 
     * Useful for administrative control or coordinated shutdowns.
     */
    void force_open();

    /**
     * @brief Reset the circuit to closed state
     * 
     * Clears all failure history and allows normal operation.
     */
    void reset();

    /**
     * @brief Get current circuit breaker statistics
     * 
     * @return CircuitBreakerStats with current state and metrics
     */
    [[nodiscard]] CircuitBreakerStats get_stats() const;

    /**
     * @brief Update configuration
     * 
     * @param config New configuration settings
     */
    void update_config(const CircuitBreakerConfig& config);

    /**
     * @brief Get current configuration
     * 
     * @return Current CircuitBreakerConfig
     */
    [[nodiscard]] const CircuitBreakerConfig& get_config() const;

private:
    /**
     * @brief Transition to open state
     */
    void transition_to_open();

    /**
     * @brief Transition to half-open state  
     */
    void transition_to_half_open();

    /**
     * @brief Transition to closed state
     */
    void transition_to_closed();

    /**
     * @brief Check if circuit should transition to open based on failure rate
     */
    [[nodiscard]] bool should_open_on_failure_rate() const;

    /**
     * @brief Update failure rate calculation
     */
    void update_failure_rate();

    // Configuration
    CircuitBreakerConfig m_config;

    // State management
    mutable std::mutex m_state_mutex;
    std::atomic<CircuitState> m_state{CircuitState::CLOSED};
    std::atomic<size_t> m_consecutive_failures{0};
    std::atomic<size_t> m_consecutive_successes{0};
    
    // Statistics  
    std::atomic<size_t> m_total_requests{0};
    std::atomic<size_t> m_successful_requests{0};
    std::atomic<size_t> m_failed_requests{0};
    std::atomic<size_t> m_rejected_requests{0};
    
    // Timing
    std::atomic<std::chrono::system_clock::time_point> m_last_failure_time;
    std::atomic<std::chrono::system_clock::time_point> m_last_success_time;
    std::atomic<std::chrono::system_clock::time_point> m_state_changed_time;
    
    // Rolling window for failure rate
    mutable std::mutex m_history_mutex;
    std::vector<std::pair<std::chrono::system_clock::time_point, bool>> m_request_history;
    std::atomic<double> m_current_failure_rate{0.0};
};

//=============================================================================
// Template Implementation
//=============================================================================

template<typename Func>
bool CircuitBreaker::execute(Func&& func, const std::string& operation_name) {
    m_total_requests++;
    
    // Check if circuit allows execution
    if (!can_execute()) {
        m_rejected_requests++;
        return false;
    }
    
    try {
        // Execute the operation
        bool success = func();
        
        if (success) {
            record_success();
            return true;
        } else {
            record_failure("Operation returned false for: " + operation_name);
            return false;
        }
        
    } catch (const std::exception& e) {
        record_failure("Exception in " + operation_name + ": " + e.what());
        return false;
    } catch (...) {
        record_failure("Unknown exception in " + operation_name);
        return false;
    }
}

} // namespace meta_prompt
} // namespace cql
