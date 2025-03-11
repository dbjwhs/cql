@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "implement a distributed task scheduler with real-time monitoring"
@context "Modern C++20 implementation using coroutines"
@context "System must work in a cloud-native environment"

@architecture "Microservices with message queue for task distribution"
@architecture "Each worker node is responsible for reporting its status"

@constraint "Must handle network partitioning gracefully"
@constraint "Recovery time < 5 seconds after failure"

@dependency "boost::asio, std::coroutine"
@dependency "nlohmann::json for message serialization"

@performance "Process up to 10,000 tasks per second distributed across nodes"
@performance "99th percentile latency < 50ms for task submission"

@security "All network traffic must be encrypted with TLS 1.3+"
@security "Role-based access control for task submission"

@complexity "O(log n) for task scheduling decisions"
@complexity "O(1) for status updates from worker nodes"

@variable "version" "1.0.0"
@variable "company" "Example Corp"

@example "Task Definition" "
struct Task {
    std::string id;
    std::string type;
    nlohmann::json payload;
    std::chrono::system_clock::time_point deadline;
    uint8_t priority;
};
"

@example "Status Update" "
struct NodeStatus {
    std::string node_id;
    double cpu_load;
    uint64_t memory_used;
    uint32_t tasks_queued;
    uint32_t tasks_processing;
    std::array<uint8_t, 3> health_indicators;
};
"

@model "claude-3-opus"
@format "markdown"

@test "Worker node failure and recovery"
@test "Load balancing with heterogeneous worker capabilities"
@test "Priority-based task scheduling"
@test "Deadline satisfaction rate > 99.9%"
@test "Security access control tests"
@test "Performance under simulated high-load conditions"