// MIT License
// Copyright (c) 2025 dbjwhs

#include "cql/meta_prompt/cost_controller.hpp"
#include "cql/project_utils.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace cql {
namespace meta_prompt {

//=============================================================================
// CostController Implementation
//=============================================================================

CostController::CostController(const CostControllerConfig& config)
    : m_config(config) {
    
    auto now = std::chrono::system_clock::now();
    m_last_daily_reset = now;
    m_last_monthly_reset = now;
    m_last_request_time = now;
    
    Logger::getInstance().log(LogLevel::INFO,
        "CostController initialized - daily_budget: $", m_config.daily_budget_usd,
        ", monthly_budget: $", m_config.monthly_budget_usd,
        ", alert_threshold: ", m_config.alert_threshold_percent, "%");
}

CostController::~CostController() {
    Logger::getInstance().log(LogLevel::INFO,
        "CostController destroyed - total_spent: $", m_total_spent.load(),
        ", total_requests: ", m_total_requests.load());
}

bool CostController::authorize_request(double estimated_cost) {
    std::lock_guard<std::mutex> lock(m_cost_mutex);
    
    check_budget_reset();
    
    // Check if estimated cost would exceed hard limits
    double potential_daily = m_daily_spent.load() + estimated_cost;
    double potential_monthly = m_monthly_spent.load() + estimated_cost;
    
    double daily_hard_limit = m_config.daily_budget_usd * (m_config.hard_limit_percent / 100.0);
    double monthly_hard_limit = m_config.monthly_budget_usd * (m_config.hard_limit_percent / 100.0);
    
    // Hard limit enforcement
    if (potential_daily > daily_hard_limit) {
        Logger::getInstance().log(LogLevel::ERROR,
            "Request denied - would exceed daily hard limit: $", potential_daily,
            " > $", daily_hard_limit);
        return false;
    }
    
    if (potential_monthly > monthly_hard_limit) {
        Logger::getInstance().log(LogLevel::ERROR,
            "Request denied - would exceed monthly hard limit: $", potential_monthly,
            " > $", monthly_hard_limit);
        return false;
    }
    
    // Check current budget status
    BudgetStatus status = get_budget_status();
    if (status == BudgetStatus::HARD_LIMIT_REACHED) {
        Logger::getInstance().log(LogLevel::ERROR,
            "Request denied - hard budget limit already reached");
        return false;
    }
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "Request authorized - estimated_cost: $", estimated_cost,
        ", remaining_daily: $", get_remaining_daily_budget(),
        ", remaining_monthly: $", get_remaining_monthly_budget());
    
    return true;
}

void CostController::record_cost(double actual_cost, const std::string& operation_type) {
    std::lock_guard<std::mutex> lock(m_cost_mutex);
    
    check_budget_reset();
    
    // Update cost totals
    m_daily_spent += actual_cost;
    m_monthly_spent += actual_cost;
    m_total_spent += actual_cost;
    
    // Update request counters
    m_daily_requests++;
    m_monthly_requests++;
    m_total_requests++;
    
    auto now = std::chrono::system_clock::now();
    m_last_request_time = now;
    
    // Update cost breakdown
    {
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        
        if (operation_type == "compilation") {
            m_cost_breakdown.compilation_cost += actual_cost;
            m_cost_breakdown.compilation_requests++;
        } else if (operation_type == "validation") {
            m_cost_breakdown.validation_cost += actual_cost;
            m_cost_breakdown.validation_requests++;
        }
        
        m_cost_breakdown.total_cost += actual_cost;
        m_cost_breakdown.total_requests++;
    }
    
    // Update cost history for predictions
    {
        std::lock_guard<std::mutex> history_lock(m_history_mutex);
        m_cost_history.emplace_back(now, actual_cost);
        
        // Keep only recent history for efficiency (last 24 hours)
        auto cutoff = now - std::chrono::hours(24);
        m_cost_history.erase(
            std::remove_if(m_cost_history.begin(), m_cost_history.end(),
                [cutoff](const auto& entry) { return entry.first < cutoff; }),
            m_cost_history.end());
    }
    
    // Update predictions and check for alerts
    update_predictions();
    check_and_trigger_alerts();
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "Cost recorded - operation: ", operation_type,
        ", cost: $", actual_cost,
        ", daily_total: $", m_daily_spent.load(),
        ", monthly_total: $", m_monthly_spent.load());
}

UsageStatistics CostController::get_usage_statistics() const {
    std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
    
    UsageStatistics stats;
    
    // Current spending
    stats.daily_spent = m_daily_spent.load();
    stats.monthly_spent = m_monthly_spent.load();
    stats.total_spent = m_total_spent.load();
    
    // Request counts
    stats.daily_requests = m_daily_requests.load();
    stats.monthly_requests = m_monthly_requests.load();
    stats.total_requests = m_total_requests.load();
    
    // Calculate efficiency metrics
    if (stats.total_requests > 0) {
        stats.average_cost_per_request = stats.total_spent / stats.total_requests;
    }
    
    // Predictions
    stats.predicted_daily_spend = m_predicted_daily_spend.load();
    stats.predicted_monthly_spend = m_predicted_monthly_spend.load();
    
    // Timing
    stats.last_reset_time = m_last_daily_reset.load();
    
    // Cost breakdown
    stats.breakdown = m_cost_breakdown;
    
    // Calculate peak hourly spend from history
    {
        std::lock_guard<std::mutex> history_lock(m_history_mutex);
        auto now = std::chrono::system_clock::now();
        double max_hourly = 0.0;
        
        for (int hour = 0; hour < 24; ++hour) {
            auto hour_start = now - std::chrono::hours(hour + 1);
            auto hour_end = now - std::chrono::hours(hour);
            
            double hourly_spend = 0.0;
            for (const auto& entry : m_cost_history) {
                if (entry.first >= hour_start && entry.first < hour_end) {
                    hourly_spend += entry.second;
                }
            }
            max_hourly = std::max(max_hourly, hourly_spend);
        }
        stats.peak_hourly_spend = max_hourly;
    }
    
    return stats;
}

BudgetStatus CostController::get_budget_status() const {
    double daily_percent = (m_daily_spent.load() / m_config.daily_budget_usd) * 100.0;
    double monthly_percent = (m_monthly_spent.load() / m_config.monthly_budget_usd) * 100.0;
    
    double max_percent = std::max(daily_percent, monthly_percent);
    
    if (max_percent >= m_config.hard_limit_percent) {
        return BudgetStatus::HARD_LIMIT_REACHED;
    } else if (max_percent >= 100.0) {
        return BudgetStatus::BUDGET_EXCEEDED;
    } else if (max_percent >= m_config.alert_threshold_percent) {
        return BudgetStatus::APPROACHING_LIMIT;
    } else {
        return BudgetStatus::NORMAL;
    }
}

bool CostController::is_daily_budget_exceeded() const {
    return m_daily_spent.load() > m_config.daily_budget_usd;
}

bool CostController::is_monthly_budget_exceeded() const {
    return m_monthly_spent.load() > m_config.monthly_budget_usd;
}

double CostController::get_remaining_daily_budget() const {
    return std::max(0.0, m_config.daily_budget_usd - m_daily_spent.load());
}

double CostController::get_remaining_monthly_budget() const {
    return std::max(0.0, m_config.monthly_budget_usd - m_monthly_spent.load());
}

void CostController::set_alert_callback(AlertCallback callback) {
    m_alert_callback = std::move(callback);
    Logger::getInstance().log(LogLevel::INFO, "Cost alert callback registered");
}

void CostController::reset_daily_budget() {
    std::lock_guard<std::mutex> lock(m_cost_mutex);
    
    Logger::getInstance().log(LogLevel::INFO,
        "Daily budget reset - previous_spent: $", m_daily_spent.load(),
        ", requests: ", m_daily_requests.load());
    
    m_daily_spent = 0.0;
    m_daily_requests = 0;
    m_last_daily_reset = std::chrono::system_clock::now();
    
    // Reset daily portions of cost breakdown
    {
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        // Note: We keep monthly/total accumulations
    }
}

void CostController::reset_monthly_budget() {
    std::lock_guard<std::mutex> lock(m_cost_mutex);
    
    Logger::getInstance().log(LogLevel::INFO,
        "Monthly budget reset - previous_spent: $", m_monthly_spent.load(),
        ", requests: ", m_monthly_requests.load());
    
    m_monthly_spent = 0.0;
    m_monthly_requests = 0;
    m_last_monthly_reset = std::chrono::system_clock::now();
    
    // Reset monthly portions of cost breakdown
    {
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        // Keep total accumulations, reset monthly-specific data
    }
}

void CostController::update_config(const CostControllerConfig& config) {
    std::lock_guard<std::mutex> lock(m_cost_mutex);
    
    Logger::getInstance().log(LogLevel::INFO,
        "CostController configuration updated - new daily_budget: $", config.daily_budget_usd,
        ", monthly_budget: $", config.monthly_budget_usd);
    
    m_config = config;
}

const CostControllerConfig& CostController::get_config() const {
    return m_config;
}

std::string CostController::export_usage_data() const {
    auto stats = get_usage_statistics();
    
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);
    
    json << "{\n";
    json << "  \"daily_spent\": " << stats.daily_spent << ",\n";
    json << "  \"monthly_spent\": " << stats.monthly_spent << ",\n";
    json << "  \"total_spent\": " << stats.total_spent << ",\n";
    json << "  \"daily_requests\": " << stats.daily_requests << ",\n";
    json << "  \"monthly_requests\": " << stats.monthly_requests << ",\n";
    json << "  \"total_requests\": " << stats.total_requests << ",\n";
    json << "  \"average_cost_per_request\": " << stats.average_cost_per_request << ",\n";
    json << "  \"peak_hourly_spend\": " << stats.peak_hourly_spend << ",\n";
    json << "  \"predicted_daily_spend\": " << stats.predicted_daily_spend << ",\n";
    json << "  \"predicted_monthly_spend\": " << stats.predicted_monthly_spend << ",\n";
    json << "  \"budget_status\": \"" << static_cast<int>(get_budget_status()) << "\",\n";
    json << "  \"breakdown\": {\n";
    json << "    \"compilation_cost\": " << stats.breakdown.compilation_cost << ",\n";
    json << "    \"validation_cost\": " << stats.breakdown.validation_cost << ",\n";
    json << "    \"compilation_requests\": " << stats.breakdown.compilation_requests << ",\n";
    json << "    \"validation_requests\": " << stats.breakdown.validation_requests << "\n";
    json << "  }\n";
    json << "}";
    
    return json.str();
}

//=============================================================================
// Private Implementation Methods
//=============================================================================

void CostController::update_predictions() {
    double usage_rate = calculate_usage_rate();
    
    if (usage_rate > 0.0) {
        auto now = std::chrono::system_clock::now();
        auto daily_reset_time = m_last_daily_reset.load();
        auto time_until_daily_reset = std::chrono::hours(24) - (now - daily_reset_time);
        
        // Predict daily spend based on current usage rate
        double remaining_hours = std::chrono::duration<double, std::ratio<3600>>(time_until_daily_reset).count();
        remaining_hours = std::max(0.0, remaining_hours);
        
        m_predicted_daily_spend = m_daily_spent.load() + (usage_rate * remaining_hours);
        
        // Predict monthly spend (simplified - assumes current rate continues)
        auto monthly_reset_time = m_last_monthly_reset.load();
        auto time_since_monthly_reset = now - monthly_reset_time;
        double hours_this_month = std::chrono::duration<double, std::ratio<3600>>(time_since_monthly_reset).count();
        
        if (hours_this_month > 0) {
            double monthly_rate = m_monthly_spent.load() / hours_this_month;
            double hours_in_month = 24.0 * 30.0; // Approximate
            m_predicted_monthly_spend = monthly_rate * hours_in_month;
        }
    }
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "Predictions updated - daily: $", m_predicted_daily_spend.load(),
        ", monthly: $", m_predicted_monthly_spend.load(),
        ", usage_rate: $", usage_rate, "/hour");
}

void CostController::check_and_trigger_alerts() {
    if (!m_alert_callback) {
        return;
    }
    
    BudgetStatus current_status = get_budget_status();
    BudgetStatus last_status = m_last_alert_status.load();
    
    // Only trigger alerts for status changes or escalations
    if (current_status != last_status && current_status != BudgetStatus::NORMAL) {
        std::string message;
        double current_spend = std::max(m_daily_spent.load(), m_monthly_spent.load());
        double budget_limit = std::max(m_config.daily_budget_usd, m_config.monthly_budget_usd);
        
        switch (current_status) {
            case BudgetStatus::APPROACHING_LIMIT:
                message = "Approaching budget limit - consider monitoring usage";
                break;
            case BudgetStatus::BUDGET_EXCEEDED:
                message = "Budget limit exceeded - requests may be throttled";
                break;
            case BudgetStatus::HARD_LIMIT_REACHED:
                message = "Hard budget limit reached - requests will be denied";
                break;
            default:
                break;
        }
        
        if (!message.empty()) {
            CostAlert alert = generate_alert(current_status, message, current_spend, budget_limit);
            m_alert_callback(alert);
            
            Logger::getInstance().log(LogLevel::ERROR,
                "Cost alert triggered - status: ", static_cast<int>(current_status),
                ", message: ", message);
        }
        
        m_last_alert_status = current_status;
    }
}

void CostController::check_budget_reset() {
    auto now = std::chrono::system_clock::now();
    
    // Check daily reset
    auto time_since_daily_reset = now - m_last_daily_reset.load();
    if (time_since_daily_reset >= std::chrono::hours(24)) {
        reset_daily_budget();
    }
    
    // Check monthly reset (simplified - every 30 days)
    auto time_since_monthly_reset = now - m_last_monthly_reset.load();
    if (time_since_monthly_reset >= std::chrono::hours(24 * 30)) {
        reset_monthly_budget();
    }
}

double CostController::calculate_usage_rate() const {
    std::lock_guard<std::mutex> history_lock(m_history_mutex);
    
    if (m_cost_history.size() < 2) {
        return 0.0;
    }
    
    auto now = std::chrono::system_clock::now();
    auto window_start = now - m_config.prediction_window;
    
    double total_cost = 0.0;
    for (const auto& entry : m_cost_history) {
        if (entry.first >= window_start) {
            total_cost += entry.second;
        }
    }
    
    double window_hours = std::chrono::duration<double, std::ratio<3600>>(m_config.prediction_window).count();
    return total_cost / window_hours;
}

CostAlert CostController::generate_alert(BudgetStatus status,
                                        const std::string& message,
                                        double current_spend,
                                        double budget_limit) const {
    CostAlert alert;
    alert.status = status;
    alert.message = message;
    alert.current_spend = current_spend;
    alert.budget_limit = budget_limit;
    alert.percent_used = (current_spend / budget_limit) * 100.0;
    alert.timestamp = std::chrono::system_clock::now();
    
    return alert;
}

} // namespace meta_prompt
} // namespace cql
