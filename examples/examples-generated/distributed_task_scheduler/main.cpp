// MIT License
// Copyright (c) 2025 dbjwhs

#include "scheduler_core.hpp"
#include "worker_node.hpp"
#include "security_manager.hpp"
#include "message_queue.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using namespace distributed_scheduler;

// Helper function for task generation
Task create_random_task(const std::string& type = "") {
    Task task;
    
    // Generate a random task type if not specified
    if (type.empty()) {
        const std::string types[] = {"compute", "io", "network"};
        task.type = types[std::rand() % 3];
    } else {
        task.type = type;
    }
    
    // Generate payload based on task type
    if (task.type == "compute") {
        task.payload = {
            {"complexity", 1 + (std::rand() % 10)},
            {"input", std::rand() % 100}
        };
    } else if (task.type == "io") {
        task.payload = {
            {"size", 1024 * (1 + (std::rand() % 100))},
            {"write", (std::rand() % 2) == 0}
        };
    } else if (task.type == "network") {
        task.payload = {
            {"count", 1 + (std::rand() % 50)},
            {"size", 64 + (std::rand() % 1024)}
        };
    }
    
    // Set deadline (between 5 and 15 seconds from now)
    int deadline_secs = 5 + (std::rand() % 10);
    task.deadline = std::chrono::system_clock::now() + std::chrono::seconds(deadline_secs);
    
    // Set priority (1-10)
    task.priority = 1 + (std::rand() % 10);
    
    return task;
}

// Helper function to monitor system metrics
void print_system_metrics(const TaskScheduler& scheduler, const std::vector<std::shared_ptr<WorkerNode>>& workers) {
    std::cout << "=== System Metrics ===" << std::endl;
    std::cout << "Pending tasks: " << scheduler.get_pending_task_count() << std::endl;
    std::cout << "Processing tasks: " << scheduler.get_processing_task_count() << std::endl;
    std::cout << "Completed tasks: " << scheduler.get_completed_task_count() << std::endl;
    std::cout << "Deadline satisfaction rate: " << (scheduler.get_deadline_satisfaction_rate() * 100) << "%" << std::endl;
    
    std::cout << "\nWorker Nodes:" << std::endl;
    for (const auto& worker : workers) {
        NodeStatus status = worker->get_current_status();
        std::cout << "- " << status.node_id << ": CPU " << status.cpu_load 
                 << "%, Tasks: " << status.tasks_processing << " processing, " 
                 << status.tasks_queued << " queued" << std::endl;
    }
    
    std::cout << std::endl;
}

// Helper function to handle worker node failures
void simulate_worker_failure(std::vector<std::shared_ptr<WorkerNode>>& workers, TaskScheduler& scheduler) {
    if (workers.empty()) return;
    
    // Select a random worker to fail
    int index = std::rand() % workers.size();
    auto worker = workers[index];
    
    std::cout << "Simulating failure of worker node: " << worker->get_id() << std::endl;
    
    // Unregister from scheduler
    scheduler.unregister_worker(worker->get_id());
    
    // Remove from our vector
    workers.erase(workers.begin() + index);
}

// Helper function to add a new worker node
void add_worker_node(std::vector<std::shared_ptr<WorkerNode>>& workers, 
                     boost::asio::io_context& io_context,
                     TaskScheduler& scheduler) {
    // Create worker with a random type
    const std::string types[] = {"standard", "high_compute", "io_optimized", "low_resource"};
    std::string worker_type = types[std::rand() % 4];
    
    auto worker = WorkerNodeFactory::create_worker(io_context, scheduler, worker_type);
    
    std::cout << "Adding new worker node: " << worker->get_id() << " (Type: " << worker_type << ")" << std::endl;
    
    // Register with scheduler
    scheduler.register_worker(worker);
    
    // Add to our vector
    workers.push_back(worker);
}

int main() {
    // Seed random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    try {
        // Create IO context
        boost::asio::io_context io_context;
        
        // Create security manager
        auto security_manager = std::make_shared<SecurityManager>("your-jwt-secret-key");
        
        // Add some users
        security_manager->add_user("admin", "admin123", {"admin"});
        security_manager->add_user("operator", "op123", {"operator"});
        security_manager->add_user("user", "user123", {"user"});
        
        // Create secure messaging
        SecureMessaging secure_messaging(io_context, "server.crt", "server.key");
        
        // Create message queue connector
        MessageQueueConnector message_queue(io_context, secure_messaging, "localhost:5672");
        
        // Connect to message broker
        message_queue.connect();
        
        // Create task scheduler
        TaskScheduler scheduler(io_context, security_manager);
        
        // Create initial worker nodes
        std::vector<std::shared_ptr<WorkerNode>> workers;
        for (int i = 0; i < 5; ++i) {
            add_worker_node(workers, io_context, scheduler);
        }
        
        // Setup task processing callbacks
        message_queue.subscribe_to_tasks([&scheduler](const Task& task) {
            // Create security context (in a real system, this would come from authentication)
            SecurityContext context;
            context.user_id = "admin";
            context.roles = {"admin"};
            context.auth_token = "dummy-token";
            context.token_expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
            
            // Submit task to scheduler
            try {
                scheduler.submit_task(task, context);
                std::cout << "Received and submitted task: " << task.id << " (Type: " << task.type << ")" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error submitting task: " << e.what() << std::endl;
            }
        });
        
        // Subscribe to task results
        scheduler.register_task_completed_callback([&message_queue](const TaskResult& result) {
            // Publish result to message queue
            message_queue.publish_task_result(result);
            
            std::cout << "Task completed: " << result.task_id 
                      << " (Success: " << (result.success ? "Yes" : "No") 
                      << ", Time: " << result.execution_time.count() << "ms)" << std::endl;
        });
        
        // Run IO context in a separate thread
        std::thread io_thread([&io_context]() {
            try {
                io_context.run();
            } catch (const std::exception& e) {
                std::cerr << "IO context error: " << e.what() << std::endl;
            }
        });
        
        // Main simulation loop
        try {
            const int simulation_seconds = 60;
            std::cout << "Starting distributed task scheduler simulation for " << simulation_seconds << " seconds..." << std::endl;
            
            for (int second = 0; second < simulation_seconds; ++second) {
                // Generate random tasks
                if (std::rand() % 100 < 80) { // 80% chance to generate tasks each second
                    int num_tasks = 1 + (std::rand() % 5); // 1-5 tasks
                    
                    for (int i = 0; i < num_tasks; ++i) {
                        // Create a random task
                        Task task = create_random_task();
                        
                        // Create security context (in a real system, this would come from authentication)
                        SecurityContext context;
                        context.user_id = "admin";
                        context.roles = {"admin"};
                        context.auth_token = "dummy-token";
                        context.token_expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
                        
                        // Submit task to scheduler
                        try {
                            std::string task_id = scheduler.submit_task(task, context);
                            std::cout << "Generated and submitted task: " << task_id 
                                      << " (Type: " << task.type 
                                      << ", Priority: " << static_cast<int>(task.priority) << ")" << std::endl;
                        } catch (const std::exception& e) {
                            std::cerr << "Error submitting task: " << e.what() << std::endl;
                        }
                    }
                }
                
                // Simulate worker failures (rare)
                if (second > 0 && second % 15 == 0 && !workers.empty()) {
                    simulate_worker_failure(workers, scheduler);
                }
                
                // Add new workers periodically
                if (second > 0 && second % 10 == 0 && workers.size() < 10) {
                    add_worker_node(workers, io_context, scheduler);
                }
                
                // Print system metrics every few seconds
                if (second % 5 == 0) {
                    print_system_metrics(scheduler, workers);
                }
                
                // Sleep for a second
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
            std::cout << "Simulation complete. Final metrics:" << std::endl;
            print_system_metrics(scheduler, workers);
            
        } catch (const std::exception& e) {
            std::cerr << "Simulation error: " << e.what() << std::endl;
        }
        
        // Stop IO context
        io_context.stop();
        
        // Wait for IO thread to finish
        if (io_thread.joinable()) {
            io_thread.join();
        }
        
        // Cleanup
        workers.clear();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}