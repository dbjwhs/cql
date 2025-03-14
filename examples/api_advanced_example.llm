@copyright "MIT License" "2025 CQL Project Contributors"
@language "C++"
@description "implement a distributed task scheduler with worker nodes and a central coordinator"

@context "Modern C++20 implementation with concepts and coroutines"
@context "Designed for cloud deployment"
@dependency "Networking library (e.g., Boost.Asio)"
@dependency "JSON serialization (e.g., nlohmann/json)"
@dependency "Message queue protocol"

@performance "Handle 100k+ tasks per second"
@performance "Low latency task distribution (<5ms)"
@security "Secure communication between nodes with JWT"
@security "Protection against message tampering"

@model "claude-3-opus"
@max_tokens 100000
@temperature 0.7
@output_format "multiple_files"

@architecture "Message-based architecture"
@architecture "Fault-tolerant with node failure recovery"
@architecture "Horizontally scalable with dynamic worker addition/removal"
@architecture "Load balancing across worker nodes"

@pattern "Observer pattern for task status updates"
@pattern "Factory pattern for task creation"
@pattern "Strategy pattern for scheduling algorithms"

@test "Test worker node registration and heartbeat"
@test "Test task submission and completion"
@test "Test fault tolerance with worker node failure"
@test "Test load balancing across workers"
@test "Test scaling with multiple workers"
@test "Test security with authentication and authorization"

@structure "include/scheduler_core.hpp: Core scheduler interface and data structures"
@structure "include/worker_node.hpp: Worker node implementation"
@structure "include/message_queue.hpp: Message queue interface"
@structure "include/security_manager.hpp: Security and authentication"
@structure "src/scheduler_implementation.cpp: Scheduler implementation"
@structure "src/worker_implementation.cpp: Worker implementation"
@structure "src/message_queue.cpp: Message queue implementation"
@structure "src/security_implementation.cpp: Security implementation"
@structure "tests/tests.cpp: Comprehensive test suite"