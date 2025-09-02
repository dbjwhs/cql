// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <chrono>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <functional>

/**
 * @file cost_controller.hpp
 * @brief Cost management and budget control for LLM API usage
 * 
 * Implements comprehensive cost tracking, budget enforcement, and usage
 * analytics to prevent unexpected API charges and provide cost visibility
 * for LLM-powered meta-prompt compilation operations.
 */

namespace cql {
namespace meta_prompt {

/**
 * @brief Cost tracking configuration
 */
struct CostControllerConfig {
    double daily_budget_usd = 10.0;                    ///< Daily spending limit in USD
    double monthly_budget_usd = 200.0;                 ///< Monthly spending limit in USD
    double alert_threshold_percent = 80.0;             ///< Alert when reaching % of budget
    double hard_limit_percent = 95.0;                  ///< Hard stop at % of budget
    std::chrono::hours budget_reset_time{0};           ///< Hour of day to reset (0-23)
    bool enable_predictive_alerts = true;              ///< Predict budget exhaustion
    std::chrono::minutes prediction_window{60};        ///< Window for usage rate prediction
};

/**
 * @brief Cost breakdown by operation type
 */
struct CostBreakdown {
    double compilation_cost = 0.0;                     ///< Cost for prompt compilation
    double validation_cost = 0.0;                      ///< Cost for semantic validation
    double total_cost = 0.0;                          ///< Total combined cost
    size_t compilation_requests = 0;                   ///< Number of compilation requests
    size_t validation_requests = 0;                    ///< Number of validation requests
    size_t total_requests = 0;                         ///< Total API requests
};

/**
 * @brief Usage statistics for cost analysis
 */
struct UsageStatistics {
    // Current period costs
    double daily_spent = 0.0;                         ///< Amount spent today
    double monthly_spent = 0.0;                       ///< Amount spent this month
    double total_spent = 0.0;                         ///< Lifetime spending
    
    // Request counts
    size_t daily_requests = 0;                        ///< Requests made today
    size_t monthly_requests = 0;                      ///< Requests made this month
    size_t total_requests = 0;                        ///< Lifetime requests
    
    // Efficiency metrics
    double average_cost_per_request = 0.0;            ///< Cost efficiency metric
    double peak_hourly_spend = 0.0;                   ///< Highest hourly spending
    std::chrono::system_clock::time_point last_reset_time; ///< When budgets were reset
    
    // Predictions
    double predicted_daily_spend = 0.0;               ///< Projected daily total
    double predicted_monthly_spend = 0.0;             ///< Projected monthly total
    std::chrono::minutes estimated_budget_exhaustion{0}; ///< Time until budget exhausted
    
    // Cost breakdown
    CostBreakdown breakdown;                          ///< Detailed cost analysis
};

/**
 * @brief Budget status and alerts
 */
enum class BudgetStatus {
    NORMAL,                                           ///< Usage within normal limits
    APPROACHING_LIMIT,                                ///< Nearing budget threshold
    BUDGET_EXCEEDED,                                  ///< Exceeded budget limits
    HARD_LIMIT_REACHED                                ///< Hard stop enforced
};

/**
 * @brief Cost alert information
 */
struct CostAlert {
    BudgetStatus status;                              ///< Alert severity level
    std::string message;                              ///< Human-readable alert message
    double current_spend;                             ///< Current spending amount
    double budget_limit;                              ///< Applicable budget limit
    double percent_used;                              ///< Percentage of budget used
    std::chrono::system_clock::time_point timestamp;  ///< When alert was generated
};

// Forward declaration for alert callback
using AlertCallback = std::function<void(const CostAlert&)>;

/**
 * @brief Cost controller for LLM API budget management
 * 
 * CostController provides comprehensive cost tracking and budget enforcement
 * for LLM API usage. It monitors spending patterns, enforces budget limits,
 * provides predictive alerts, and offers detailed usage analytics to prevent
 * unexpected charges while maintaining service availability.
 * 
 * Key features:
 * - Real-time cost tracking with operation-level breakdown
 * - Daily and monthly budget enforcement with soft/hard limits
 * - Predictive spending analysis and budget exhaustion alerts
 * - Usage pattern analysis and cost optimization recommendations
 * - Thread-safe operations for concurrent compilation requests
 */
class CostController {
public:
    /**
     * @brief Constructor with configuration
     * 
     * @param config CostControllerConfig with budget and alert settings
     */
    explicit CostController(const CostControllerConfig& config = {});

    /**
     * @brief Destructor
     */
    ~CostController();

    /**
     * @brief Check if a request can proceed within budget limits
     * 
     * Performs pre-request budget validation to prevent exceeding limits.
     * Takes estimated cost to make informed authorization decisions.
     * 
     * @param estimated_cost Expected cost of the operation in USD
     * @return true if request is authorized within budget
     */
    [[nodiscard]] bool authorize_request(double estimated_cost);

    /**
     * @brief Record actual cost of a completed operation
     * 
     * Updates cost tracking with actual usage. Should be called after
     * every API operation to maintain accurate cost records.
     * 
     * @param actual_cost Actual cost incurred in USD
     * @param operation_type Type of operation (compilation/validation)
     */
    void record_cost(double actual_cost, const std::string& operation_type = "compilation");

    /**
     * @brief Get current usage statistics
     * 
     * @return UsageStatistics with current spending and predictions
     */
    [[nodiscard]] UsageStatistics get_usage_statistics() const;

    /**
     * @brief Get current budget status
     * 
     * @return BudgetStatus indicating current spending level
     */
    [[nodiscard]] BudgetStatus get_budget_status() const;

    /**
     * @brief Check if daily budget limit is exceeded
     * 
     * @return true if daily spending exceeds configured limit
     */
    [[nodiscard]] bool is_daily_budget_exceeded() const;

    /**
     * @brief Check if monthly budget limit is exceeded
     * 
     * @return true if monthly spending exceeds configured limit
     */
    [[nodiscard]] bool is_monthly_budget_exceeded() const;

    /**
     * @brief Get remaining daily budget
     * 
     * @return Amount of daily budget remaining in USD
     */
    [[nodiscard]] double get_remaining_daily_budget() const;

    /**
     * @brief Get remaining monthly budget
     * 
     * @return Amount of monthly budget remaining in USD
     */
    [[nodiscard]] double get_remaining_monthly_budget() const;

    /**
     * @brief Register callback for cost alerts
     * 
     * @param callback Function to call when alerts are generated
     */
    void set_alert_callback(AlertCallback callback);

    /**
     * @brief Reset daily budget and usage counters
     * 
     * Called automatically at configured reset time or manually for testing.
     */
    void reset_daily_budget();

    /**
     * @brief Reset monthly budget and usage counters
     * 
     * Called automatically at month boundaries or manually for testing.
     */
    void reset_monthly_budget();

    /**
     * @brief Update configuration
     * 
     * @param config New configuration settings
     */
    void update_config(const CostControllerConfig& config);

    /**
     * @brief Get current configuration
     * 
     * @return Current CostControllerConfig
     */
    [[nodiscard]] const CostControllerConfig& get_config() const;

    /**
     * @brief Export usage data for analysis
     * 
     * @return JSON string with detailed usage statistics
     */
    [[nodiscard]] std::string export_usage_data() const;

private:
    /**
     * @brief Update predictive spending analysis
     */
    void update_predictions();

    /**
     * @brief Check for budget alerts and trigger callbacks
     */
    void check_and_trigger_alerts();

    /**
     * @brief Check if budget reset is needed
     */
    void check_budget_reset();

    /**
     * @brief Calculate current usage rate
     */
    [[nodiscard]] double calculate_usage_rate() const;

    /**
     * @brief Generate cost alert
     */
    [[nodiscard]] CostAlert generate_alert(BudgetStatus status, 
                                           const std::string& message,
                                           double current_spend,
                                           double budget_limit) const;

    // Configuration
    CostControllerConfig m_config;

    // Thread safety
    mutable std::mutex m_cost_mutex;
    mutable std::mutex m_stats_mutex;

    // Cost tracking
    std::atomic<double> m_daily_spent{0.0};
    std::atomic<double> m_monthly_spent{0.0};
    std::atomic<double> m_total_spent{0.0};

    // Request tracking
    std::atomic<size_t> m_daily_requests{0};
    std::atomic<size_t> m_monthly_requests{0};
    std::atomic<size_t> m_total_requests{0};

    // Cost breakdown
    CostBreakdown m_cost_breakdown;

    // Timing
    std::atomic<std::chrono::system_clock::time_point> m_last_daily_reset;
    std::atomic<std::chrono::system_clock::time_point> m_last_monthly_reset;
    std::atomic<std::chrono::system_clock::time_point> m_last_request_time;

    // Usage history for predictions
    mutable std::mutex m_history_mutex;
    std::vector<std::pair<std::chrono::system_clock::time_point, double>> m_cost_history;

    // Predictions
    std::atomic<double> m_predicted_daily_spend{0.0};
    std::atomic<double> m_predicted_monthly_spend{0.0};

    // Alert system
    AlertCallback m_alert_callback;
    std::atomic<BudgetStatus> m_last_alert_status{BudgetStatus::NORMAL};
};

} // namespace meta_prompt
} // namespace cql
