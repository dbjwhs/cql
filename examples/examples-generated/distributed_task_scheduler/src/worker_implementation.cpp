// MIT License
// Copyright (c) 2025 dbjwhs

#include "worker_node.hpp"
#include <chrono>
#include <random>
#include <thread>

namespace distributed_scheduler {

WorkerNode::WorkerNode(boost::asio::io_context& io_context,
                      const std::string& node_id,
                      TaskScheduler& scheduler)
    : io_context_(io_context), node_id_(node_id), scheduler_(scheduler) {
    
    // Initialize timers
    resource_update_timer_ = std::make_unique<boost::asio::steady_timer>(io_context_);
    heartbeat_timer_ = std::make_unique<boost::asio::steady_timer>(io_context_);
}

WorkerNode::~WorkerNode() {
    stop();
}

void WorkerNode::start() {
    running_ = true;
    
    // Start resource simulation
    update_status_metrics();
    
    // Start heartbeat
    send_heartbeat();
}

void WorkerNode::stop() {
    running_ = false;
    
    // Cancel timers
    if (resource_update_timer_) {
        // Call cancel without error code parameter
        resource_update_timer_->cancel();
    }

    if (heartbeat_timer_) {
        // Call cancel without error code parameter
        heartbeat_timer_->cancel();
    }

    // Cancel all pending tasks
    std::unique_lock<std::mutex> lock(tasks_mutex_);
    while (!pending_tasks_.empty()) {
        auto task = pending_tasks_.front();
        pending_tasks_.pop();

        // Report task as failed
        TaskResult result;
        result.task_id = task->id;
        result.success = false;
        result.error_message = "Worker node shut down";
        result.execution_time = std::chrono::milliseconds(0);

        scheduler_.notify_task_completed(result);
    }

    // Cancel all active tasks (actual cancellation handled by coroutines)
    active_tasks_.clear();
    task_coroutines_.clear();
}

bool WorkerNode::assign_task(std::shared_ptr<Task> task) {
    if (!running_) return false;

    // Check if we can accept another task
    if (active_tasks_.size() >= max_concurrent_tasks_) return false;

    // Add to pending tasks queue
    {
        std::unique_lock<std::mutex> lock(tasks_mutex_);
        pending_tasks_.push(task);
    }

    // Notify worker thread to process the task
    task_available_cv_.notify_one();

    // Start processing if we have capacity
    process_next_task();

    return true;
}

bool WorkerNode::cancel_task(const std::string& task_id) {
    std::unique_lock<std::mutex> lock(tasks_mutex_);

    // Check if the task is still pending
    std::queue<std::shared_ptr<Task>> temp_queue;
    bool found = false;

    while (!pending_tasks_.empty()) {
        auto task = pending_tasks_.front();
        pending_tasks_.pop();

        if (task->id == task_id) {
            found = true;

            // Report task as cancelled
            TaskResult result;
            result.task_id = task_id;
            result.success = false;
            result.error_message = "Task cancelled";
            result.execution_time = std::chrono::milliseconds(0);

            scheduler_.notify_task_completed(result);
        } else {
            temp_queue.push(task);
        }
    }

    // Restore pending tasks
    pending_tasks_ = std::move(temp_queue);

    // Check if the task is currently active
    if (!found) {
        auto it = active_tasks_.find(task_id);
        if (it != active_tasks_.end()) {
            // Remove from active tasks
            active_tasks_.erase(it);

            // Remove any associated coroutine
            task_coroutines_.erase(task_id);

            // Report task as cancelled
            TaskResult result;
            result.task_id = task_id;
            result.success = false;
            result.error_message = "Task cancelled";
            result.execution_time = std::chrono::milliseconds(0);

            scheduler_.notify_task_completed(result);

            found = true;
        }
    }

    return found;
}

NodeStatus WorkerNode::get_current_status() const {
    NodeStatus status;
    status.node_id = node_id_;
    status.cpu_load = cpu_load_.load();
    status.memory_used = memory_used_.load();
    status.tasks_queued = pending_tasks_.size();
    status.tasks_processing = active_tasks_.size();

    // Copy health indicators
    status.health_indicators[0] = health_indicators_[0].load();
    status.health_indicators[1] = health_indicators_[1].load();
    status.health_indicators[2] = health_indicators_[2].load();

    status.last_heartbeat = std::chrono::system_clock::now();

    return status;
}

void WorkerNode::set_max_concurrent_tasks(uint32_t max_tasks) {
    max_concurrent_tasks_ = max_tasks;
}

bool WorkerNode::can_accept_task(const Task& task) const {
    // Check if the worker is running
    if (!running_) return false;

    // Check if we're at capacity
    if (active_tasks_.size() >= max_concurrent_tasks_) return false;

    // Get current status
    NodeStatus status = get_current_status();

    // Check if the worker is healthy
    if (!status.is_available()) return false;

    // Check if we have time to complete the task before its deadline
    auto time_remaining = task.time_until_deadline();

    // Estimate task completion time based on task type and payload
    std::chrono::milliseconds estimated_time(0);

    if (task.type == "compute") {
        estimated_time = std::chrono::milliseconds(200 + (task.payload.value("complexity", 1) * 50));
    } else if (task.type == "io") {
        estimated_time = std::chrono::milliseconds(100 + (task.payload.value("size", 1) / 1024));
    } else if (task.type == "network") {
        estimated_time = std::chrono::milliseconds(150 + (task.payload.value("count", 1) * 10));
    } else {
        // Default processing time
        estimated_time = std::chrono::milliseconds(300);
    }

    // Add a safety margin
    estimated_time = estimated_time * 2;

    // Check if we have enough time
    return time_remaining > estimated_time;
}

double WorkerNode::get_suitability_score(const Task& task) const {
    // Base score is the health score
    NodeStatus status = get_current_status();
    double score = status.get_health_score();

    // Task type affinity
    if (task.type == "compute" && cpu_load_ < 50.0) {
        score += 10.0;
    } else if (task.type == "io" && memory_used_ < 50000000) {
        score += 10.0;
    }

    // Deadline factor - higher score for tasks with tight deadlines
    auto time_remaining = task.time_until_deadline();
    if (time_remaining < std::chrono::seconds(1)) {
        score += 20.0;
    } else if (time_remaining < std::chrono::seconds(5)) {
        score += 10.0;
    }

    // Load balancing factor - lower score for busy workers
    score -= active_tasks_.size() * 5.0;

    return score;
}

TaskCoroutine WorkerNode::execute_task(std::shared_ptr<Task> task) {
    // Track execution time
    auto start_time = std::chrono::steady_clock::now();

    try {
        // Process the task using the WorkSimulator
        // This will do the actual "work" with progress reporting
        auto progress_callback = [](double /*progress*/) {
            // Could use this to update task status in a more detailed implementation
        };

        auto result_data = co_await WorkSimulator(*task, progress_callback);

        // Calculate execution time
        auto end_time = std::chrono::steady_clock::now();
        auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        // Prepare successful result
        TaskResult result;
        result.task_id = task->id;
        result.success = true;
        result.result_data = std::move(result_data);
        result.execution_time = execution_time;

        co_return result;
    } catch (const std::exception& e) {
        // Calculate execution time even for failures
        auto end_time = std::chrono::steady_clock::now();
        auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        // Prepare failure result
        TaskResult result;
        result.task_id = task->id;
        result.success = false;
        result.error_message = e.what();
        result.execution_time = execution_time;

        co_return result;
    } catch (...) {
        // Calculate execution time even for failures
        auto end_time = std::chrono::steady_clock::now();
        auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        // Prepare failure result
        TaskResult result;
        result.task_id = task->id;
        result.success = false;
        result.error_message = "Unknown error occurred during task execution";
        result.execution_time = execution_time;

        co_return result;
    }
}

void WorkerNode::process_next_task() {
    std::unique_lock<std::mutex> lock(tasks_mutex_);

    // Check if we're at capacity or have no pending tasks
    if (active_tasks_.size() >= max_concurrent_tasks_ || pending_tasks_.empty()) {
        return;
    }

    // Get the next task
    auto task = pending_tasks_.front();
    pending_tasks_.pop();

    // Add to active tasks
    active_tasks_[task->id] = task;

    // Execute the task as a coroutine
    auto coroutine = execute_task(task);
    task_coroutines_[task->id] = std::move(coroutine);

    // Schedule a task to collect the result when ready
    boost::asio::post(io_context_, [this, task_id = task->id]() {
        auto it = task_coroutines_.find(task_id);
        if (it != task_coroutines_.end()) {
            // Get the result from the coroutine
            TaskResult result = it->second.get_result();

            // Notify the scheduler that the task is complete
            scheduler_.notify_task_completed(result);

            // Remove from active tasks and coroutines
            {
                std::unique_lock<std::mutex> lock(tasks_mutex_);
                active_tasks_.erase(task_id);
                task_coroutines_.erase(task_id);
            }

            // Process the next task if we have any
            process_next_task();
        }
    });
}

void WorkerNode::update_status_metrics() {
    if (!running_) return;

    // Simulate CPU load fluctuations
    std::uniform_real_distribution<double> cpu_dist(-5.0, 5.0);
    double cpu_delta = cpu_dist(random_generator_);
    cpu_load_ = std::clamp(cpu_load_.load() + cpu_delta, 10.0, 90.0);

    // Simulate memory usage fluctuations
    std::uniform_int_distribution<uint64_t> mem_dist(-1000000, 1000000);
    int64_t mem_delta = mem_dist(random_generator_);
    memory_used_ = std::max<uint64_t>(1000000, memory_used_.load() + mem_delta);

    // Simulate health indicator fluctuations
    for (size_t i = 0; i < health_indicators_.size(); ++i) {
        std::uniform_int_distribution<int> health_dist(-2, 2);
        int health_delta = health_dist(random_generator_);
        health_indicators_[i] = static_cast<uint8_t>(
            std::clamp<int>(health_indicators_[i].load() + health_delta, 50, 100)
        );
    }

    // Schedule next update
    resource_update_timer_->expires_after(std::chrono::milliseconds(500));
    resource_update_timer_->async_wait([this](const boost::system::error_code& error) {
        if (!error) {
            update_status_metrics();
        }
    });
}

void WorkerNode::send_heartbeat() {
    if (!running_) return;

    // Get current status and send to scheduler
    NodeStatus status = get_current_status();
    scheduler_.update_node_status(status);

    // Schedule next heartbeat
    heartbeat_timer_->expires_after(std::chrono::seconds(1));
    heartbeat_timer_->async_wait([this](const boost::system::error_code& error) {
        if (!error) {
            send_heartbeat();
        }
    });
}

} // namespace distributed_scheduler
