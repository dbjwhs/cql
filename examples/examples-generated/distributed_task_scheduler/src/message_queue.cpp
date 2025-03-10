// MIT License
// Copyright (c) 2025 dbjwhs

#include "message_queue.hpp"
#include <chrono>
#include <thread>
#include <future>
#include <random>

namespace distributed_scheduler {

MessageQueueConnector::MessageQueueConnector(boost::asio::io_context& io_context,
                                           SecureMessaging& /*secure_messaging*/,
                                           const std::string& broker_address)
    : io_context_(io_context), 
      broker_address_(broker_address),
      last_operation_time_(std::chrono::system_clock::now()) {
    
    // Initialize reconnect timer
    reconnect_timer_ = std::make_unique<boost::asio::steady_timer>(io_context_);
}

void MessageQueueConnector::connect() {
    // In a real implementation, this would establish a connection to the message broker
    // For this example, we'll simulate connection success/failure
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0, 1);
    
    if (dist(gen) < 0.9) { // 90% success rate
        connected_ = true;
        connection_failures_ = 0;
        last_operation_time_ = std::chrono::system_clock::now();
        
        // Start processing messages
        process_incoming_messages();
    } else {
        connected_ = false;
        connection_failures_++;
        
        // Handle connection failure
        handle_connection_failure();
    }
}

void MessageQueueConnector::disconnect() {
    // In a real implementation, this would close the connection to the message broker
    connected_ = false;
    
    // Cancel reconnect timer
    if (reconnect_timer_) {
        reconnect_timer_->cancel();
    }
}

bool MessageQueueConnector::publish_task(const Task& task) {
    if (!connected_) {
        return false;
    }
    
    try {
        // In a real implementation, serialize the task and send it to the message broker
        // For this example, we'll simulate publication success/failure
        
        // Convert task to JSON
        nlohmann::json task_json = {
            {"id", task.id},
            {"type", task.type},
            {"payload", task.payload},
            {"deadline", std::chrono::system_clock::to_time_t(task.deadline)},
            {"priority", task.priority}
        };
        
        // In a real implementation, we would encrypt and sign the message here
        // using the secure_messaging_ service
        
        // Simulate successful publication
        last_operation_time_ = std::chrono::system_clock::now();
        return true;
    } catch (const std::exception&) {
        // In a real implementation, handle the error appropriately
        return false;
    }
}

bool MessageQueueConnector::publish_task_cancellation(const std::string& task_id) {
    if (!connected_) {
        return false;
    }
    
    try {
        // In a real implementation, send a cancellation message to the message broker
        // For this example, we'll simulate publication success/failure
        
        // Convert cancellation to JSON
        nlohmann::json cancel_json = {
            {"task_id", task_id},
            {"action", "cancel"},
            {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
        };
        
        // Simulate successful publication
        last_operation_time_ = std::chrono::system_clock::now();
        return true;
    } catch (const std::exception&) {
        // In a real implementation, handle the error appropriately
        return false;
    }
}

bool MessageQueueConnector::publish_task_result(const TaskResult& result) {
    if (!connected_) {
        return false;
    }
    
    try {
        // In a real implementation, send the result to the message broker
        // For this example, we'll simulate publication success/failure
        
        // Convert result to JSON
        nlohmann::json result_json = {
            {"task_id", result.task_id},
            {"success", result.success},
            {"result_data", result.result_data},
            {"error_message", result.error_message},
            {"execution_time_ms", result.execution_time.count()},
            {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
        };
        
        // Simulate successful publication
        last_operation_time_ = std::chrono::system_clock::now();
        return true;
    } catch (const std::exception&) {
        // In a real implementation, handle the error appropriately
        return false;
    }
}

bool MessageQueueConnector::publish_node_status(const NodeStatus& status) {
    if (!connected_) {
        return false;
    }
    
    try {
        // In a real implementation, send the status to the message broker
        // For this example, we'll simulate publication success/failure
        
        // Convert status to JSON
        nlohmann::json status_json = {
            {"node_id", status.node_id},
            {"cpu_load", status.cpu_load},
            {"memory_used", status.memory_used},
            {"tasks_queued", status.tasks_queued},
            {"tasks_processing", status.tasks_processing},
            {"health_indicators", {
                status.health_indicators[0],
                status.health_indicators[1],
                status.health_indicators[2]
            }},
            {"last_heartbeat", std::chrono::system_clock::to_time_t(status.last_heartbeat)}
        };
        
        // Simulate successful publication
        last_operation_time_ = std::chrono::system_clock::now();
        return true;
    } catch (const std::exception&) {
        // In a real implementation, handle the error appropriately
        return false;
    }
}

void MessageQueueConnector::subscribe_to_tasks(std::function<void(const Task&)> callback) {
    task_callback_ = std::move(callback);
}

void MessageQueueConnector::subscribe_to_task_cancellations(std::function<void(const std::string&)> callback) {
    task_cancellation_callback_ = std::move(callback);
}

void MessageQueueConnector::subscribe_to_task_results(std::function<void(const TaskResult&)> callback) {
    task_result_callback_ = std::move(callback);
}

void MessageQueueConnector::subscribe_to_node_status(std::function<void(const NodeStatus&)> callback) {
    node_status_callback_ = std::move(callback);
}

bool MessageQueueConnector::is_connected() const {
    return connected_;
}

std::chrono::system_clock::time_point MessageQueueConnector::last_successful_operation() const {
    return last_operation_time_;
}

void MessageQueueConnector::process_incoming_messages() {
    if (!connected_) {
        return;
    }
    
    // In a real implementation, this would continuously receive and process messages
    // For this example, we'll simulate receiving messages periodically
    
    // Schedule periodic message processing
    auto process_interval = std::chrono::milliseconds(50);
    
    boost::asio::post(io_context_, [this, process_interval]() {
        if (!connected_) {
            return;
        }
        
        // Simulate receiving messages with low probability
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(0, 1);
        
        if (dist(gen) < 0.1) { // 10% chance of receiving a message
            // Determine message type
            std::uniform_int_distribution<> type_dist(0, 3);
            int message_type = type_dist(gen);
            
            switch (message_type) {
                case 0: // Task message
                    if (task_callback_) {
                        // Create a simulated task
                        Task task;
                        task.id = "sim_" + std::to_string(std::rand());
                        task.type = "compute";
                        task.payload = {{"complexity", 5}};
                        task.deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
                        task.priority = 5;
                        
                        task_callback_(task);
                    }
                    break;
                
                case 1: // Task cancellation message
                    if (task_cancellation_callback_) {
                        // Create a simulated task ID
                        std::string task_id = "sim_" + std::to_string(std::rand());
                        
                        task_cancellation_callback_(task_id);
                    }
                    break;
                
                case 2: // Task result message
                    if (task_result_callback_) {
                        // Create a simulated task result
                        TaskResult result;
                        result.task_id = "sim_" + std::to_string(std::rand());
                        result.success = true;
                        result.result_data = {{"output", 42}};
                        result.execution_time = std::chrono::milliseconds(500);
                        
                        task_result_callback_(result);
                    }
                    break;
                
                case 3: // Node status message
                    if (node_status_callback_) {
                        // Create a simulated node status
                        NodeStatus status;
                        status.node_id = "sim_node_" + std::to_string(std::rand());
                        status.cpu_load = 50.0;
                        status.memory_used = 100000000;
                        status.tasks_queued = 5;
                        status.tasks_processing = 3;
                        status.health_indicators = {80, 85, 90};
                        status.last_heartbeat = std::chrono::system_clock::now();
                        
                        node_status_callback_(status);
                    }
                    break;
            }
        }
        
        // Simulate periodic connection issues with very low probability
        if (dist(gen) < 0.01) { // 1% chance of connection issue
            connected_ = false;
            handle_connection_failure();
            return;
        }
        
        // Reschedule
        boost::asio::steady_timer timer(io_context_, process_interval);
        timer.async_wait([this](const boost::system::error_code& error) {
            if (!error) {
                process_incoming_messages();
            }
        });
    });
}

void MessageQueueConnector::handle_connection_failure() {
    // Increment failure counter
    connection_failures_++;
    
    // Calculate backoff time based on failures (exponential backoff)
    int failures = connection_failures_.load();
    int capped_failures = std::min(failures, 10);
    auto backoff = std::chrono::milliseconds(
        100 * static_cast<int>(std::pow(2, capped_failures))
    );
    
    // Schedule reconnection attempt
    reconnect_timer_->expires_after(backoff);
    reconnect_timer_->async_wait([this](const boost::system::error_code& error) {
        if (!error) {
            reconnect();
        }
    });
}

void MessageQueueConnector::reconnect() {
    // Attempt to reconnect
    connect();
}

} // namespace distributed_scheduler
