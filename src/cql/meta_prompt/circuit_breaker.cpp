// MIT License
// Copyright (c) 2025 dbjwhs

#include "cql/meta_prompt/circuit_breaker.hpp"
#include "cql/project_utils.hpp"
#include <algorithm>

namespace cql {
namespace meta_prompt {

//=============================================================================
// CircuitBreaker Implementation
//=============================================================================

CircuitBreaker::CircuitBreaker(const CircuitBreakerConfig& config)
    : m_config(config) {
    
    auto now = std::chrono::system_clock::now();
    m_state_changed_time = now;
    m_last_failure_time = now;
    m_last_success_time = now;
    
    Logger::getInstance().log(LogLevel::INFO,
        "CircuitBreaker initialized - failure_threshold: ", m_config.failure_threshold,
        ", timeout: ", m_config.timeout_duration.count(), "s");
}

CircuitBreaker::~CircuitBreaker() {
    Logger::getInstance().log(LogLevel::DEBUG, "CircuitBreaker destroyed");
}

bool CircuitBreaker::can_execute() const {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    
    CircuitState current_state = m_state.load();
    auto now = std::chrono::system_clock::now();
    
    switch (current_state) {
        case CircuitState::CLOSED:
            return true;
            
        case CircuitState::OPEN: {
            // Check if timeout period has elapsed
            auto time_since_state_change = now - m_state_changed_time.load();
            if (time_since_state_change >= m_config.timeout_duration) {
                // Transition to half-open for testing recovery
                const_cast<CircuitBreaker*>(this)->transition_to_half_open();
                return true;
            }
            return false;
        }
        
        case CircuitState::HALF_OPEN:
            return true;
            
        default:
            return false;
    }
}

void CircuitBreaker::record_success() {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    
    m_successful_requests++;
    m_consecutive_failures = 0;
    m_consecutive_successes++;
    m_last_success_time = std::chrono::system_clock::now();
    
    // Update request history for failure rate calculation
    {
        std::lock_guard<std::mutex> history_lock(m_history_mutex);
        m_request_history.emplace_back(m_last_success_time.load(), true);
        update_failure_rate();
    }
    
    CircuitState current_state = m_state.load();
    
    if (current_state == CircuitState::HALF_OPEN) {
        // Check if we have enough consecutive successes to close the circuit
        if (m_consecutive_successes >= m_config.success_threshold) {
            transition_to_closed();
        }
    }
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "CircuitBreaker recorded success - consecutive_successes: ", m_consecutive_successes.load(),
        ", state: ", static_cast<int>(current_state));
}

void CircuitBreaker::record_failure(const std::string& error_message) {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    
    m_failed_requests++;
    m_consecutive_successes = 0;
    m_consecutive_failures++;
    m_last_failure_time = std::chrono::system_clock::now();
    
    // Update request history for failure rate calculation
    {
        std::lock_guard<std::mutex> history_lock(m_history_mutex);
        m_request_history.emplace_back(m_last_failure_time.load(), false);
        update_failure_rate();
    }
    
    CircuitState current_state = m_state.load();
    
    Logger::getInstance().log(LogLevel::ERROR,
        "CircuitBreaker recorded failure - consecutive_failures: ", m_consecutive_failures.load(),
        ", error: ", error_message.empty() ? "unspecified" : error_message);
    
    // Decide if circuit should open
    bool should_open = false;
    
    // Check consecutive failure threshold
    if (m_consecutive_failures >= m_config.failure_threshold) {
        should_open = true;
        Logger::getInstance().log(LogLevel::ERROR,
            "CircuitBreaker failure threshold exceeded: ", m_consecutive_failures.load(),
            " >= ", m_config.failure_threshold);
    }
    
    // Check failure rate threshold (if we have enough requests)
    if (!should_open && should_open_on_failure_rate()) {
        should_open = true;
        Logger::getInstance().log(LogLevel::ERROR,
            "CircuitBreaker failure rate threshold exceeded: ", m_current_failure_rate.load(),
            " >= ", m_config.failure_rate_threshold);
    }
    
    if (should_open && current_state != CircuitState::OPEN) {
        transition_to_open();
    }
}

void CircuitBreaker::force_open() {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    
    if (m_state.load() != CircuitState::OPEN) {
        Logger::getInstance().log(LogLevel::INFO, "CircuitBreaker manually forced to OPEN state");
        transition_to_open();
    }
}

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    
    Logger::getInstance().log(LogLevel::INFO, "CircuitBreaker reset to initial state");
    
    // Reset all counters and state
    m_consecutive_failures = 0;
    m_consecutive_successes = 0;
    m_total_requests = 0;
    m_successful_requests = 0;
    m_failed_requests = 0;
    m_rejected_requests = 0;
    
    auto now = std::chrono::system_clock::now();
    m_last_failure_time = now;
    m_last_success_time = now;
    
    // Clear request history
    {
        std::lock_guard<std::mutex> history_lock(m_history_mutex);
        m_request_history.clear();
        m_current_failure_rate = 0.0;
    }
    
    transition_to_closed();
}

CircuitBreakerStats CircuitBreaker::get_stats() const {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    
    CircuitBreakerStats stats;
    stats.current_state = m_state.load();
    stats.total_requests = m_total_requests.load();
    stats.successful_requests = m_successful_requests.load();
    stats.failed_requests = m_failed_requests.load();
    stats.rejected_requests = m_rejected_requests.load();
    stats.last_failure_time = m_last_failure_time.load();
    stats.last_success_time = m_last_success_time.load();
    stats.state_changed_time = m_state_changed_time.load();
    stats.current_failure_rate = m_current_failure_rate.load();
    
    return stats;
}

void CircuitBreaker::update_config(const CircuitBreakerConfig& config) {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    
    Logger::getInstance().log(LogLevel::INFO,
        "CircuitBreaker configuration updated - new failure_threshold: ", config.failure_threshold,
        ", timeout: ", config.timeout_duration.count(), "s");
        
    m_config = config;
}

const CircuitBreakerConfig& CircuitBreaker::get_config() const {
    return m_config;
}

//=============================================================================
// Private Implementation Methods
//=============================================================================

void CircuitBreaker::transition_to_open() {
    m_state = CircuitState::OPEN;
    m_state_changed_time = std::chrono::system_clock::now();
    
    Logger::getInstance().log(LogLevel::ERROR,
        "CircuitBreaker transitioned to OPEN state - requests will be rejected");
}

void CircuitBreaker::transition_to_half_open() {
    m_state = CircuitState::HALF_OPEN;
    m_state_changed_time = std::chrono::system_clock::now();
    m_consecutive_successes = 0; // Reset for testing recovery
    
    Logger::getInstance().log(LogLevel::INFO,
        "CircuitBreaker transitioned to HALF_OPEN state - testing recovery");
}

void CircuitBreaker::transition_to_closed() {
    m_state = CircuitState::CLOSED;
    m_state_changed_time = std::chrono::system_clock::now();
    m_consecutive_failures = 0; // Reset failure count
    
    Logger::getInstance().log(LogLevel::INFO,
        "CircuitBreaker transitioned to CLOSED state - normal operation resumed");
}

bool CircuitBreaker::should_open_on_failure_rate() const {
    // Must have minimum number of requests before rate-based decisions
    if (m_total_requests < m_config.minimum_requests) {
        return false;
    }
    
    double current_rate = m_current_failure_rate.load();
    return current_rate >= m_config.failure_rate_threshold;
}

void CircuitBreaker::update_failure_rate() {
    auto now = std::chrono::system_clock::now();
    auto cutoff_time = now - m_config.rolling_window;
    
    // Remove old entries outside the rolling window
    m_request_history.erase(
        std::remove_if(m_request_history.begin(), m_request_history.end(),
            [cutoff_time](const auto& entry) {
                return entry.first < cutoff_time;
            }),
        m_request_history.end());
    
    if (m_request_history.empty()) {
        m_current_failure_rate = 0.0;
        return;
    }
    
    // Calculate failure rate within rolling window
    size_t total_in_window = m_request_history.size();
    size_t failures_in_window = std::count_if(
        m_request_history.begin(), m_request_history.end(),
        [](const auto& entry) { return !entry.second; }); // !success means failure
    
    m_current_failure_rate = static_cast<double>(failures_in_window) / total_in_window;
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "CircuitBreaker failure rate updated: ", m_current_failure_rate.load(),
        " (", failures_in_window, "/", total_in_window, " in window)");
}

} // namespace meta_prompt
} // namespace cql
