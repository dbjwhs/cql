// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <coroutine>
#include <future>
#include <atomic>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace distributed_scheduler {

// Forward declarations
class TaskScheduler;
class WorkerNode;
class TaskDistributor;
class SecurityManager;

/**
 * @brief Task definition with metadata for scheduling and execution
 */
struct Task {
    std::string id;
    std::string type;
    nlohmann::json payload;
    std::chrono::system_clock::time_point deadline;
    uint8_t priority;
    
    // Additional fields for internal tracking
    std::string assigned_node_id;
    std::chrono::system_clock::time_point created_at{std::chrono::system_clock::now()};
    std::chrono::system_clock::time_point started_at;
    std::optional<std::chrono::system_clock::time_point> completed_at;
    
    // Convenience methods
    bool is_expired() const {
        return std::chrono::system_clock::now() > deadline;
    }
    
    std::chrono::milliseconds time_until_deadline() const {
        auto now = std::chrono::system_clock::now();
        if (now >= deadline) return std::chrono::milliseconds(0);
        return std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
    }
};

/**
 * @brief Worker node status information for monitoring and scheduling decisions
 */
struct NodeStatus {
    std::string node_id;
    double cpu_load;
    uint64_t memory_used;
    uint32_t tasks_queued;
    uint32_t tasks_processing;
    std::array<uint8_t, 3> health_indicators;
    std::chrono::system_clock::time_point last_heartbeat;
    
    // Derived metrics
    double get_health_score() const {
        // A simple health score calculation; can be made more sophisticated
        double health_avg = (health_indicators[0] + health_indicators[1] + health_indicators[2]) / 3.0;
        double load_factor = 1.0 - (cpu_load / 100.0);
        return health_avg * load_factor;
    }
    
    bool is_available() const {
        return cpu_load < 90.0 && 
               health_indicators[0] > 20 &&
               health_indicators[1] > 20 &&
               health_indicators[2] > 20;
    }
    
    // For worker capacity estimation (scheduling decisions)
    uint32_t estimated_capacity() const {
        if (!is_available()) return 0;
        
        // Base capacity scaled by CPU availability
        const uint32_t base_capacity = 100;
        double cpu_availability = 1.0 - (cpu_load / 100.0);
        return static_cast<uint32_t>(base_capacity * cpu_availability);
    }
};

/**
 * @brief Represents a task execution result
 */
struct TaskResult {
    std::string task_id;
    bool success;
    nlohmann::json result_data;
    std::string error_message;
    std::chrono::milliseconds execution_time;
};

/**
 * @brief Security context for authorization
 */
struct SecurityContext {
    std::string user_id;
    std::vector<std::string> roles;
    std::string auth_token;
    std::chrono::system_clock::time_point token_expiry;
    
    bool has_role(const std::string& role) const {
        return std::find(roles.begin(), roles.end(), role) != roles.end();
    }
    
    bool is_valid() const {
        return !user_id.empty() && !auth_token.empty() && 
               (std::chrono::system_clock::now() < token_expiry);
    }
};

/**
 * @brief Exception for security violations
 */
class SecurityException : public std::runtime_error {
public:
    explicit SecurityException(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @brief Exception for scheduling failures
 */
class SchedulingException : public std::runtime_error {
public:
    explicit SchedulingException(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @brief Awaitable type for task completion
 */
class TaskAwaiter {
public:
    explicit TaskAwaiter(std::string task_id, TaskScheduler& scheduler);
    
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> handle);
    TaskResult await_resume();
    
private:
    std::string task_id_;
    TaskScheduler& scheduler_;
    TaskResult result_;
    bool is_ready_ = false;
};

/**
 * @brief Custom comparator for task priority queue
 */
struct TaskPriorityComparator {
    bool operator()(const std::shared_ptr<Task>& a, const std::shared_ptr<Task>& b) const {
        // First compare priority (higher value = higher priority)
        if (a->priority != b->priority) {
            return a->priority < b->priority;
        }
        
        // Then compare deadline (earlier deadline = higher priority)
        if (a->deadline != b->deadline) {
            return a->deadline > b->deadline;
        }
        
        // Finally compare creation time (older = higher priority)
        return a->created_at > b->created_at;
    }
};

// Type aliases for clarity
using TaskQueue = std::priority_queue<
    std::shared_ptr<Task>, 
    std::vector<std::shared_ptr<Task>>, 
    TaskPriorityComparator
>;

using TaskResultCallback = std::function<void(const TaskResult&)>;
using NodeStatusCallback = std::function<void(const NodeStatus&)>;

/**
 * @brief Core scheduler class that manages task distribution and monitoring
 */
class TaskScheduler {
public:
    TaskScheduler(boost::asio::io_context& io_context,
                  std::shared_ptr<SecurityManager> security_manager);
    
    ~TaskScheduler();
    
    // Task submission and management
    std::string submit_task(const Task& task, const SecurityContext& security_ctx);
    TaskAwaiter await_task(const std::string& task_id);
    bool cancel_task(const std::string& task_id, const SecurityContext& security_ctx);
    
    // Task retrieval and status
    std::optional<Task> get_task(const std::string& task_id) const;
    std::vector<Task> get_pending_tasks() const;
    std::optional<TaskResult> get_task_result(const std::string& task_id) const;
    
    // Worker node management
    void register_worker(std::shared_ptr<WorkerNode> worker);
    void unregister_worker(const std::string& worker_id);
    void update_node_status(const NodeStatus& status);
    std::vector<NodeStatus> get_all_node_statuses() const;
    
    // Status monitoring and metrics
    uint32_t get_pending_task_count() const;
    uint32_t get_processing_task_count() const;
    uint32_t get_completed_task_count() const;
    double get_deadline_satisfaction_rate() const;
    
    // Event registration
    void register_task_completed_callback(TaskResultCallback callback);
    void register_node_status_callback(NodeStatusCallback callback);
    
    // For task result notification (called by worker nodes)
    void notify_task_completed(const TaskResult& result);
    
private:
    // Core scheduler logic
    std::shared_ptr<WorkerNode> select_best_worker_for_task(const Task& task);
    void schedule_pending_tasks();
    void check_worker_timeouts();
    void handle_worker_failure(const std::string& worker_id);
    void rebalance_tasks();
    
    // Internal tracking structures
    mutable std::shared_mutex tasks_mutex_;
    std::unordered_map<std::string, std::shared_ptr<Task>> tasks_;
    TaskQueue pending_tasks_;
    std::unordered_map<std::string, TaskResult> completed_tasks_;
    
    mutable std::shared_mutex workers_mutex_;
    std::unordered_map<std::string, std::shared_ptr<WorkerNode>> workers_;
    std::unordered_map<std::string, NodeStatus> worker_statuses_;
    
    // Callback registrations
    std::vector<TaskResultCallback> task_completed_callbacks_;
    std::vector<NodeStatusCallback> node_status_callbacks_;
    
    // Task tracking metrics
    std::atomic<uint32_t> total_tasks_completed_{0};
    std::atomic<uint32_t> tasks_completed_on_time_{0};
    
    // IO and timing
    boost::asio::io_context& io_context_;
    std::unique_ptr<boost::asio::steady_timer> worker_check_timer_;
    std::unique_ptr<boost::asio::steady_timer> task_scheduler_timer_;
    std::atomic<bool> running_timer_callbacks_{true};
    
    // Security
    std::shared_ptr<SecurityManager> security_manager_;
};

} // namespace distributed_scheduler
