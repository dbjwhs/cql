// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "scheduler_core.hpp"
#include <atomic>
#include <queue>
#include <thread>
#include <condition_variable>
#include <coroutine>
#include <random>
#include <boost/asio/ssl.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace distributed_scheduler {

/**
 * @brief Represents a resumable task computation using C++20 coroutines
 */
struct TaskCoroutine {
    struct promise_type {
        TaskResult result;
        
        TaskCoroutine get_return_object() {
            return TaskCoroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        auto initial_suspend() { return std::suspend_never{}; }
        
        auto final_suspend() noexcept { return std::suspend_always{}; }
        
        void unhandled_exception() {
            std::string error_message;
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const std::exception& e) {
                error_message = e.what();
            } catch (...) {
                error_message = "Unknown exception";
            }
            
            result.success = false;
            result.error_message = error_message;
        }
        
        void return_value(TaskResult r) {
            result = std::move(r);
        }
    };
    
    std::coroutine_handle<promise_type> handle{nullptr};
    
    TaskCoroutine() = default; // Add default constructor
    
    TaskCoroutine(std::coroutine_handle<promise_type> h) : handle(h) {}
    
    // Add move constructor and move assignment
    TaskCoroutine(TaskCoroutine&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    
    TaskCoroutine& operator=(TaskCoroutine&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    
    // Delete copy constructor and copy assignment
    TaskCoroutine(const TaskCoroutine&) = delete;
    TaskCoroutine& operator=(const TaskCoroutine&) = delete;
    
    ~TaskCoroutine() {
        if (handle) handle.destroy();
    }
    
    TaskResult get_result() {
        return handle.promise().result;
    }
};

/**
 * @brief Custom awaitable for simulating work with progress reporting
 */
class WorkSimulator {
public:
    WorkSimulator(const Task& task, std::function<void(double)> progress_callback)
        : task_(task), progress_callback_(std::move(progress_callback)) {}
    
    bool await_ready() const noexcept { return false; }
    
    void await_suspend(std::coroutine_handle<> handle) {
        std::thread([this, handle]() mutable {
            // Simulate work based on task type and payload size
            const auto work_duration = calculate_work_duration();
            const auto start_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::milliseconds(0);
            
            // Report progress at intervals
            while (elapsed < work_duration) {
                // Sleep for a short interval
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Calculate and report progress
                elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time);
                double progress = static_cast<double>(elapsed.count()) / work_duration.count();
                progress_callback_(progress);
            }
            
            // Work completed, resume the coroutine
            handle.resume();
        }).detach();
    }
    
    nlohmann::json await_resume() const {
        // Generate a result based on the task
        nlohmann::json result;
        result["processed"] = true;
        result["task_type"] = task_.type;
        
        // Add some task-specific result data
        if (task_.type == "compute") {
            result["computation_result"] = task_.payload.value("input", 0) * 2;
        } else if (task_.type == "io") {
            result["bytes_processed"] = task_.payload.value("size", 0);
        } else if (task_.type == "network") {
            result["packets_sent"] = task_.payload.value("count", 0);
            result["latency_ms"] = 15 + (std::rand() % 30);
        }
        
        return result;
    }
    
private:
    const Task& task_;
    std::function<void(double)> progress_callback_;
    
    std::chrono::milliseconds calculate_work_duration() const {
        // Base duration on task type and complexity
        std::chrono::milliseconds base_duration(0);
        
        if (task_.type == "compute") {
            base_duration = std::chrono::milliseconds(200 + (task_.payload.value("complexity", 1) * 50));
        } else if (task_.type == "io") {
            base_duration = std::chrono::milliseconds(100 + (task_.payload.value("size", 1) / 1024));
        } else if (task_.type == "network") {
            base_duration = std::chrono::milliseconds(150 + (task_.payload.value("count", 1) * 10));
        } else {
            // Default processing time
            base_duration = std::chrono::milliseconds(300);
        }
        
        // Add some randomness to simulate real-world variations
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> variation(-50, 100);
        
        return base_duration + std::chrono::milliseconds(variation(gen));
    }
};

/**
 * @brief Represents a worker node in the distributed system
 */
class WorkerNode : public std::enable_shared_from_this<WorkerNode> {
public:
    WorkerNode(boost::asio::io_context& io_context,
               const std::string& node_id,
               TaskScheduler& scheduler);
    
    ~WorkerNode();
    
    // Initialize and start the worker
    void start();
    void stop();
    
    // Task management
    bool assign_task(std::shared_ptr<Task> task);
    bool cancel_task(const std::string& task_id);
    
    // Status reporting
    NodeStatus get_current_status() const;
    
    // Worker configuration
    void set_max_concurrent_tasks(uint32_t max_tasks);
    std::string get_id() const { return node_id_; }
    
    // For scheduling decisions
    bool can_accept_task(const Task& task) const;
    double get_suitability_score(const Task& task) const;
    
private:
    // Task execution
    TaskCoroutine execute_task(std::shared_ptr<Task> task);
    void process_next_task();
    void update_status_metrics();
    void send_heartbeat();
    
    // Internal state
    boost::asio::io_context& io_context_;
    std::string node_id_;
    TaskScheduler& scheduler_;
    
    // Task queues and processing
    std::mutex tasks_mutex_;
    std::queue<std::shared_ptr<Task>> pending_tasks_;
    std::unordered_map<std::string, std::shared_ptr<Task>> active_tasks_;
    std::unordered_map<std::string, TaskCoroutine> task_coroutines_;
    
    // Status tracking
    std::atomic<uint32_t> max_concurrent_tasks_{5};
    std::atomic<double> cpu_load_{0.0};
    std::atomic<uint64_t> memory_used_{0};
    std::array<std::atomic<uint8_t>, 3> health_indicators_{{100, 100, 100}};
    
    // Resource simulation
    std::unique_ptr<boost::asio::steady_timer> resource_update_timer_;
    std::unique_ptr<boost::asio::steady_timer> heartbeat_timer_;
    std::mt19937 random_generator_{std::random_device{}()};
    
    // Worker state
    std::atomic<bool> running_{false};
    std::condition_variable task_available_cv_;
};

/**
 * @brief A factory class for creating worker nodes
 */
class WorkerNodeFactory {
public:
    static std::shared_ptr<WorkerNode> create_worker(
        boost::asio::io_context& io_context,
        TaskScheduler& scheduler,
        const std::string& node_type = "standard") {
        
        // Generate a unique ID for the worker
        auto uuid = boost::uuids::random_generator()();
        std::string node_id = boost::uuids::to_string(uuid);
        
        // Create the worker
        auto worker = std::make_shared<WorkerNode>(io_context, node_id, scheduler);
        
        // Configure based on node type
        if (node_type == "high_compute") {
            worker->set_max_concurrent_tasks(10);
        } else if (node_type == "io_optimized") {
            worker->set_max_concurrent_tasks(15);
        } else if (node_type == "low_resource") {
            worker->set_max_concurrent_tasks(3);
        } else {
            // Standard worker
            worker->set_max_concurrent_tasks(5);
        }
        
        return worker;
    }
};

} // namespace distributed_scheduler
