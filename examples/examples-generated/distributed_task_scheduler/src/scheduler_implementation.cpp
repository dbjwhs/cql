// MIT License
// Copyright (c) 2025 dbjwhs

#include "scheduler_core.hpp"
#include "worker_node.hpp"
#include "security_manager.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace distributed_scheduler {

//===== TaskAwaiter Implementation =====

TaskAwaiter::TaskAwaiter(std::string task_id, TaskScheduler& scheduler)
    : task_id_(std::move(task_id)), scheduler_(scheduler), is_ready_(false) {
    
    // Check if the task result is already available
    auto result = scheduler_.get_task_result(task_id_);
    if (result.has_value()) {
        result_ = result.value();
        is_ready_ = true;
    }
}

bool TaskAwaiter::await_ready() const noexcept {
    return is_ready_;
}

void TaskAwaiter::await_suspend(std::coroutine_handle<> handle) {
    // Register a callback that will resume the coroutine when the task completes
    scheduler_.register_task_completed_callback(
        [this, handle](const TaskResult& result) mutable {
            if (result.task_id == this->task_id_) {
                this->result_ = result;
                this->is_ready_ = true;
                handle.resume();
            }
        }
    );
}

TaskResult TaskAwaiter::await_resume() {
    return result_;
}

//===== TaskScheduler Implementation =====

TaskScheduler::TaskScheduler(boost::asio::io_context& io_context,
                           std::shared_ptr<SecurityManager> security_manager)
    : io_context_(io_context), security_manager_(std::move(security_manager)) {
    
    // Initialize timers for periodic operations
    worker_check_timer_ = std::make_unique<boost::asio::steady_timer>(io_context_);
    task_scheduler_timer_ = std::make_unique<boost::asio::steady_timer>(io_context_);
    
    // Schedule periodic worker health checks
    auto check_interval = std::chrono::seconds(1);
    worker_check_timer_->expires_after(check_interval);
    worker_check_timer_->async_wait(
        [this, check_interval](const boost::system::error_code& error) {
            if (!error && running_timer_callbacks_) {
                this->check_worker_timeouts();
                
                // Reschedule
                if (running_timer_callbacks_) {
                    worker_check_timer_->expires_after(check_interval);
                    worker_check_timer_->async_wait(
                        [this, check_interval](const boost::system::error_code& error) {
                            if (!error && running_timer_callbacks_) {
                                this->check_worker_timeouts();
                            }
                        }
                    );
                }
            }
        }
    );
    
    // Schedule periodic task scheduling
    auto schedule_interval = std::chrono::milliseconds(100);
    task_scheduler_timer_->expires_after(schedule_interval);
    task_scheduler_timer_->async_wait(
        [this, schedule_interval](const boost::system::error_code& error) {
            if (!error && running_timer_callbacks_) {
                this->schedule_pending_tasks();
                
                // Reschedule
                if (running_timer_callbacks_) {
                    task_scheduler_timer_->expires_after(schedule_interval);
                    task_scheduler_timer_->async_wait(
                        [this, schedule_interval](const boost::system::error_code& error) {
                            if (!error && running_timer_callbacks_) {
                                this->schedule_pending_tasks();
                            }
                        }
                    );
                }
            }
        }
    );
}

TaskScheduler::~TaskScheduler() {
    // Stop timer callbacks
    running_timer_callbacks_ = false;
    
    // Stop timers
    if (worker_check_timer_) {
        worker_check_timer_->cancel();
    }
    
    if (task_scheduler_timer_) {
        task_scheduler_timer_->cancel();
    }
}

std::string TaskScheduler::submit_task(const Task& task, const SecurityContext& security_ctx) {
    // Validate security context
    if (!security_ctx.is_valid()) {
        throw SecurityException("Invalid security context");
    }
    
    // Check permission to submit this task type
    if (!security_manager_->can_submit_task(security_ctx, task.type)) {
        throw SecurityException("Unauthorized to submit tasks of type: " + task.type);
    }
    
    // Generate a unique task ID if not provided
    std::string task_id = task.id;
    if (task_id.empty()) {
        auto uuid = boost::uuids::random_generator()();
        task_id = boost::uuids::to_string(uuid);
    }
    
    // Create a shared_ptr copy of the task with the assigned ID
    auto task_ptr = std::make_shared<Task>(task);
    task_ptr->id = task_id;
    
    // Add to the pending tasks queue
    {
        std::unique_lock<std::shared_mutex> lock(tasks_mutex_);
        tasks_[task_id] = task_ptr;
        pending_tasks_.push(task_ptr);
    }
    
    return task_id;
}

TaskAwaiter TaskScheduler::await_task(const std::string& task_id) {
    return TaskAwaiter(task_id, *this);
}

bool TaskScheduler::cancel_task(const std::string& task_id, const SecurityContext& security_ctx) {
    // Validate security context
    if (!security_ctx.is_valid()) {
        throw SecurityException("Invalid security context");
    }
    
    // Check permission to cancel this task
    if (!security_manager_->can_cancel_task(security_ctx, task_id)) {
        throw SecurityException("Unauthorized to cancel task: " + task_id);
    }
    
    std::unique_lock<std::shared_mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        // Task not found
        return false;
    }
    
    auto task_ptr = it->second;
    
    // Check if the task is already assigned to a worker
    if (!task_ptr->assigned_node_id.empty()) {
        std::shared_lock<std::shared_mutex> workers_lock(workers_mutex_);
        auto worker_it = workers_.find(task_ptr->assigned_node_id);
        if (worker_it != workers_.end()) {
            // Ask the worker to cancel the task
            worker_it->second->cancel_task(task_id);
        }
    }
    
    // Remove from our tracking structures
    tasks_.erase(it);
    
    // We can't easily remove from the priority queue, so we'll just let it be
    // and filter it out when we process the queue
    
    return true;
}

std::optional<Task> TaskScheduler::get_task(const std::string& task_id) const {
    std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        return std::nullopt;
    }
    
    return *it->second;
}

std::vector<Task> TaskScheduler::get_pending_tasks() const {
    std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
    
    std::vector<Task> result;
    result.reserve(tasks_.size());
    
    for (const auto& [_, task_ptr] : tasks_) {
        if (task_ptr->assigned_node_id.empty() && !task_ptr->completed_at.has_value()) {
            result.push_back(*task_ptr);
        }
    }
    
    return result;
}

std::optional<TaskResult> TaskScheduler::get_task_result(const std::string& task_id) const {
    std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
    
    auto it = completed_tasks_.find(task_id);
    if (it == completed_tasks_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

void TaskScheduler::register_worker(std::shared_ptr<WorkerNode> worker) {
    std::unique_lock<std::shared_mutex> lock(workers_mutex_);
    
    workers_[worker->get_id()] = worker;
    
    // Initialize worker status
    NodeStatus initial_status = worker->get_current_status();
    worker_statuses_[worker->get_id()] = initial_status;
    
    // Start the worker
    worker->start();
}

void TaskScheduler::unregister_worker(const std::string& worker_id) {
    std::unique_lock<std::shared_mutex> lock(workers_mutex_);
    
    auto it = workers_.find(worker_id);
    if (it != workers_.end()) {
        // Stop the worker gracefully
        it->second->stop();
        
        // Remove from our tracking structures
        workers_.erase(it);
        worker_statuses_.erase(worker_id);
    }
}

void TaskScheduler::update_node_status(const NodeStatus& status) {
    {
        std::unique_lock<std::shared_mutex> lock(workers_mutex_);
        worker_statuses_[status.node_id] = status;
    }
    
    // Notify status change to registered callbacks
    for (const auto& callback : node_status_callbacks_) {
        callback(status);
    }
}

std::vector<NodeStatus> TaskScheduler::get_all_node_statuses() const {
    std::shared_lock<std::shared_mutex> lock(workers_mutex_);
    
    std::vector<NodeStatus> result;
    result.reserve(worker_statuses_.size());
    
    for (const auto& [_, status] : worker_statuses_) {
        result.push_back(status);
    }
    
    return result;
}

uint32_t TaskScheduler::get_pending_task_count() const {
    std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
    
    uint32_t count = 0;
    for (const auto& [_, task_ptr] : tasks_) {
        if (task_ptr->assigned_node_id.empty() && !task_ptr->completed_at.has_value()) {
            count++;
        }
    }
    
    return count;
}

uint32_t TaskScheduler::get_processing_task_count() const {
    std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
    
    uint32_t count = 0;
    for (const auto& [_, task_ptr] : tasks_) {
        if (!task_ptr->assigned_node_id.empty() && !task_ptr->completed_at.has_value()) {
            count++;
        }
    }
    
    return count;
}

uint32_t TaskScheduler::get_completed_task_count() const {
    return total_tasks_completed_.load();
}

double TaskScheduler::get_deadline_satisfaction_rate() const {
    uint32_t completed = total_tasks_completed_.load();
    if (completed == 0) {
        return 1.0; // No tasks completed yet
    }
    
    uint32_t on_time = tasks_completed_on_time_.load();
    return static_cast<double>(on_time) / static_cast<double>(completed);
}

void TaskScheduler::register_task_completed_callback(TaskResultCallback callback) {
    task_completed_callbacks_.push_back(std::move(callback));
}

void TaskScheduler::register_node_status_callback(NodeStatusCallback callback) {
    node_status_callbacks_.push_back(std::move(callback));
}

void TaskScheduler::notify_task_completed(const TaskResult& result) {
    bool deadline_met = false;
    
    {
        std::unique_lock<std::shared_mutex> lock(tasks_mutex_);
        
        // Update task status
        auto it = tasks_.find(result.task_id);
        if (it != tasks_.end()) {
            auto& task_ptr = it->second;
            task_ptr->completed_at = std::chrono::system_clock::now();
            
            // Check if the deadline was met
            deadline_met = *task_ptr->completed_at <= task_ptr->deadline;
            
            // Store the result
            completed_tasks_[result.task_id] = result;
        }
    }
    
    // Update metrics
    total_tasks_completed_.fetch_add(1, std::memory_order_relaxed);
    if (deadline_met) {
        tasks_completed_on_time_.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Notify completion to registered callbacks
    for (const auto& callback : task_completed_callbacks_) {
        callback(result);
    }
}

std::shared_ptr<WorkerNode> TaskScheduler::select_best_worker_for_task(const Task& task) {
    std::shared_lock<std::shared_mutex> lock(workers_mutex_);
    
    std::shared_ptr<WorkerNode> best_worker;
    double best_score = -1.0;
    
    for (const auto& [worker_id, worker] : workers_) {
        // Skip workers that can't accept this task
        if (!worker->can_accept_task(task)) {
            continue;
        }
        
        // Get worker's suitability score for this task
        double score = worker->get_suitability_score(task);
        
        // Update best worker if this one is better
        if (score > best_score) {
            best_score = score;
            best_worker = worker;
        }
    }
    
    return best_worker;
}

void TaskScheduler::schedule_pending_tasks() {
    std::unique_lock<std::shared_mutex> lock(tasks_mutex_);
    
    // Process pending tasks in priority order
    while (!pending_tasks_.empty()) {
        auto task_ptr = pending_tasks_.top();
        
        // Skip tasks that are already assigned or completed
        if (!task_ptr->assigned_node_id.empty() || task_ptr->completed_at.has_value()) {
            pending_tasks_.pop();
            continue;
        }
        
        // Select the best worker for this task
        auto worker = select_best_worker_for_task(*task_ptr);
        
        if (worker) {
            // Assign the task to the worker
            if (worker->assign_task(task_ptr)) {
                // Update task assignment
                task_ptr->assigned_node_id = worker->get_id();
                task_ptr->started_at = std::chrono::system_clock::now();
                
                // Move to next task
                pending_tasks_.pop();
            } else {
                // Worker couldn't accept the task, try the next best worker
                // (by leaving it in the queue for now)
                break;
            }
        } else {
            // No suitable worker found, stop scheduling for now
            break;
        }
    }
}

void TaskScheduler::check_worker_timeouts() {
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> failed_workers;
    
    {
        std::shared_lock<std::shared_mutex> lock(workers_mutex_);
        
        // Check for workers that haven't sent a heartbeat recently
        for (const auto& [worker_id, status] : worker_statuses_) {
            auto time_since_heartbeat = std::chrono::duration_cast<std::chrono::seconds>(
                now - status.last_heartbeat
            );
            
            // If no heartbeat for 5 seconds, consider the worker failed
            if (time_since_heartbeat > std::chrono::seconds(5)) {
                failed_workers.push_back(worker_id);
            }
        }
    }
    
    // Handle failed workers
    for (const auto& worker_id : failed_workers) {
        handle_worker_failure(worker_id);
    }
    
    // If we have worker failures, rebalance tasks
    if (!failed_workers.empty()) {
        rebalance_tasks();
    }
}

void TaskScheduler::handle_worker_failure(const std::string& worker_id) {
    std::unique_lock<std::shared_mutex> workers_lock(workers_mutex_);
    
    // Remove the worker from our tracking structures
    workers_.erase(worker_id);
    worker_statuses_.erase(worker_id);
    
    // Release workers_lock before acquiring tasks_lock to avoid deadlock
    workers_lock.unlock();
    
    std::unique_lock<std::shared_mutex> tasks_lock(tasks_mutex_);
    
    // Find tasks assigned to the failed worker
    for (auto& [task_id, task_ptr] : tasks_) {
        if (task_ptr->assigned_node_id == worker_id && !task_ptr->completed_at.has_value()) {
            // Reset the task assignment so it can be rescheduled
            task_ptr->assigned_node_id.clear();
            
            // Add back to the pending queue
            pending_tasks_.push(task_ptr);
        }
    }
}

void TaskScheduler::rebalance_tasks() {
    // Simple implementation: just let the scheduler reassign tasks
    // This is automatically handled by schedule_pending_tasks()
    
    // A more sophisticated implementation could selectively move tasks
    // from heavily loaded workers to less loaded ones
}

} // namespace distributed_scheduler