# Distributed Task Scheduler with Real-time Monitoring

> MIT License
> Copyright (c) 2025 dbjwhs

A modern C++20 distributed task scheduler designed for cloud-native environments, featuring coroutines, message queue integration, and real-time monitoring capabilities.

## Features

- **Modern C++20 Implementation**: Leverages coroutines for asynchronous task handling
- **Cloud-Native Architecture**: Designed for microservice environments
- **Distributed Task Scheduling**: O(log n) scheduling decisions across worker nodes
- **Real-time Monitoring**: Worker status tracking with O(1) update complexity
- **Self-healing**: Automatic recovery from network partitions and node failures
- **High Performance**: Processes up to 10,000 tasks per second with <50ms latency (99th percentile)
- **Role-based Security**: TLS 1.3+ encryption and role-based access control

## System Requirements

- Modern C++ compiler with C++20 support (GCC 10+ or Clang 12+)
- Boost libraries (particularly Asio)
- OpenSSL 1.1.1 or newer (for TLS 1.3 support)
- nlohmann::json library
- JWT-CPP library
- CMake 3.15 or newer (for building)
- Google Test and Google Mock (for testing)

## Architecture

The system is designed with the following components:

1. **Task Scheduler**: Central component that manages task distribution based on priority, deadline, and worker capabilities
2. **Worker Nodes**: Distributed processing units that execute tasks and report status
3. **Security Manager**: Handles authentication, authorization, and access control
4. **Message Queue Connector**: Facilitates communication between components in a distributed environment

### Task Flow

1. Client submits a task with type, payload, deadline, and priority
2. Scheduler authenticates request and places task in priority queue
3. Scheduler selects optimal worker node based on capabilities and load
4. Worker node processes task asynchronously using coroutines
5. Results are reported back through the message queue
6. System monitors health and performance metrics in real-time

## Building the Project

```bash
# Clone the repository
git clone https://github.com/yourusername/distributed-task-scheduler.git
cd distributed-task-scheduler

# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake libboost-all-dev libssl-dev \
    nlohmann-json3-dev libjwt-cpp-dev libgtest-dev doxygen graphviz

# Create build directory
mkdir -p build && cd build

# Configure and build the project
cmake ..
cmake --build .

# Run tests
ctest --output-on-failure

# Generate documentation (optional)
cmake --build . --target documentation
```

Alternatively, you can use the provided build script:

```bash
chmod +x build.sh
./build.sh         # Release build with tests
./build.sh --debug --docs  # Debug build with documentation
```

## Usage Examples

### Basic Task Scheduling

{% raw %}
```cpp
#include "scheduler_core.hpp"
#include "worker_node.hpp"
#include "security_manager.hpp"
#include <iostream>

using namespace distributed_scheduler;

int main() {
    // Create IO context
    boost::asio::io_context io_context;

    // Initialize security manager
    auto security_manager = std::make_shared<SecurityManager>("your-secret-key");
    security_manager->add_user("admin", "admin123", {"admin"});

    // Create task scheduler
    TaskScheduler scheduler(io_context, security_manager);

    // Create and register worker nodes
    auto worker = WorkerNodeFactory::create_worker(io_context, scheduler);
    scheduler.register_worker(worker);

    // Create security context for task submission
    SecurityContext context;
    context.user_id = "admin";
    context.roles = {"admin"};
    context.auth_token = security_manager->authenticate("admin", "admin123").auth_token;

    // Create and submit a task
    Task task;
    task.type = "compute";
    task.payload = {{"complexity", 5}, {"input", 42}};
    task.deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
    task.priority = 5;

    std::string task_id = scheduler.submit_task(task, context);

    // Run the IO context
    io_context.run();

    return 0;
}
```
{% endraw %}

### Monitoring Worker Nodes

```cpp
// Register for node status updates
scheduler.register_node_status_callback([](const NodeStatus& status) {
    std::cout << "Node: " << status.node_id << std::endl;
    std::cout << "  CPU Load: " << status.cpu_load << "%" << std::endl;
    std::cout << "  Memory Used: " << (status.memory_used / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Tasks: " << status.tasks_processing << " processing, "
              << status.tasks_queued << " queued" << std::endl;
    std::cout << "  Health Score: " << status.get_health_score() << std::endl;
});
```

### Using Coroutines for Task Processing

```cpp
TaskCoroutine process_data(std::shared_ptr<Task> task) {
    // Start timing
    auto start_time = std::chrono::steady_clock::now();

    // Simulate work with progress reporting
    auto result_data = co_await WorkSimulator(*task, [](double progress) {
        std::cout << "Progress: " << (progress * 100) << "%" << std::endl;
    });

    // Calculate execution time
    auto end_time = std::chrono::steady_clock::now();
    auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    // Prepare result
    TaskResult result;
    result.task_id = task->id;
    result.success = true;
    result.result_data = std::move(result_data);
    result.execution_time = execution_time;

    co_return result;
}
```

## Configuration Options

The system can be configured through environment variables or configuration files:

- `TASK_SCHEDULER_WORKERS`: Number of worker nodes to create (default: 5)
- `TASK_SCHEDULER_MAX_QUEUE`: Maximum task queue size (default: 10000)
- `TASK_SCHEDULER_MESSAGE_BROKER`: Address of the message broker (default: localhost:5672)
- `TASK_SCHEDULER_TLS_CERT`: Path to TLS certificate file
- `TASK_SCHEDULER_TLS_KEY`: Path to TLS key file
- `TASK_SCHEDULER_JWT_SECRET`: Secret key for JWT token generation

## Performance Tuning

For optimal performance in production environments:

1. Increase the number of worker nodes based on available hardware
2. Adjust task priorities to ensure critical tasks are processed first
3. Set appropriate deadlines for different task types
4. Configure worker node specialization for heterogeneous workloads
5. Monitor and adjust health check thresholds based on observed metrics

## License

MIT License. See the LICENSE file for details.