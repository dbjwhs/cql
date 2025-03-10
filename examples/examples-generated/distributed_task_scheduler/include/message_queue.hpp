// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "scheduler_core.hpp"
#include "security_manager.hpp"
#include <functional>
#include <chrono>
#include <atomic>
#include <boost/asio.hpp>

namespace distributed_scheduler {

/**
 * @brief Manages message queue connections for task distribution
 */
class MessageQueueConnector {
public:
    MessageQueueConnector(boost::asio::io_context& io_context,
                         SecureMessaging& secure_messaging,
                         const std::string& broker_address);
    
    // Message queue operations
    void connect();
    void disconnect();
    
    // Task distribution
    bool publish_task(const Task& task);
    bool publish_task_cancellation(const std::string& task_id);
    bool publish_task_result(const TaskResult& result);
    
    // Status updates
    bool publish_node_status(const NodeStatus& status);
    
    // Queue subscription
    void subscribe_to_tasks(std::function<void(const Task&)> callback);
    void subscribe_to_task_cancellations(std::function<void(const std::string&)> callback);
    void subscribe_to_task_results(std::function<void(const TaskResult&)> callback);
    void subscribe_to_node_status(std::function<void(const NodeStatus&)> callback);
    
    // Health and diagnostics
    bool is_connected() const;
    std::chrono::system_clock::time_point last_successful_operation() const;

private:
    // Internal message handling
    void process_incoming_messages();
    void handle_connection_failure();
    void reconnect();
    
    // State tracking
    boost::asio::io_context& io_context_;
    // SecureMessaging reference is kept for future implementation of secure messaging
    std::string broker_address_;
    std::atomic<bool> connected_{false};
    std::chrono::system_clock::time_point last_operation_time_;
    
    // Connection management
    std::unique_ptr<boost::asio::steady_timer> reconnect_timer_;
    std::atomic<int> connection_failures_{0};
    
    // Message callbacks
    std::function<void(const Task&)> task_callback_;
    std::function<void(const std::string&)> task_cancellation_callback_;
    std::function<void(const TaskResult&)> task_result_callback_;
    std::function<void(const NodeStatus&)> node_status_callback_;
};

} // namespace distributed_scheduler
