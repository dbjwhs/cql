# CQL Phase 2 Features

This document describes the new features added in Phase 2 of the Claude Query Language (CQL).

## New Directive Types

### Architecture Directives
The `@architecture` directive specifies high-level architectural patterns or requirements:

```
@architecture "Microservices with message queue for task distribution"
@architecture "Event-driven architecture with reactive components"
```

### Constraint Directives
The `@constraint` directive imposes specific limitations or requirements:

```
@constraint "Must handle network partitioning gracefully"
@constraint "Recovery time < 5 seconds after failure"
```

### Example Directives
The `@example` directive provides code examples for better context:

```
@example "Task Definition" "
struct Task {
    std::string id;
    std::string type;
    nlohmann::json payload;
    std::chrono::system_clock::time_point deadline;
    uint8_t priority;
};
"
```

### Security Directives
The `@security` directive specifies security requirements:

```
@security "All network traffic must be encrypted with TLS 1.3+"
@security "Role-based access control for task submission"
```

### Complexity Directives
The `@complexity` directive sets algorithmic complexity requirements:

```
@complexity "O(log n) for task scheduling decisions"
@complexity "O(1) for status updates from worker nodes"
```

### Model Directives
The `@model` directive specifies which LLM model to target:

```
@model "claude-3-opus"
@model "claude-3-sonnet"
```

### Format Directives
The `@format` directive controls output formatting:

```
@format "markdown"
@format "json"
```

### Variable Directives
The `@variable` directive declares template variables for interpolation:

```
@variable "version" "1.0.0"
@variable "company" "Example Corp"
```

## Template Variables and Interpolation

Phase 2 adds support for variable interpolation within query text:

1. Declare variables with the `@variable` directive:
   ```
   @variable "max_size" "1000"
   ```

2. Reference variables in examples or other text with `${var_name}`:
   ```
   @example "Queue Usage" "
   ThreadSafeQueue<int> queue(${max_size});
   queue.push(42);
   "
   ```

3. Variables are automatically replaced during compilation.

## Query Validation

Phase 2 adds a robust validation system that enforces rules for query structure:

- **Required Directives**: Ensures essential directives are present
  - `@language` and `@description` are required by default

- **Exclusive Directives**: Warns when multiple instances of directives that should be unique are present
  - `@model` and `@format` should appear only once

- **Dependency Rules**: Ensures related directives are used together
  - `@architecture` works best with `@context`

- **Custom Rules**: Validates based on project-specific requirements
  - A default rule warns when test cases are missing

Validation produces warnings and errors with detailed messages to help users craft effective queries.

## Output Formatting

The compiler now supports different output formats:

- **Markdown**: Default human-readable format
- **JSON**: Structured format for programmatic consumption

Output formatting is controlled by the `@format` directive.

## Model Targeting

Queries can be targeted to specific Claude models using the `@model` directive:

```
@model "claude-3-opus"
```

## Example Full Query with Phase 2 Features

```
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

@model "claude-3-opus"
@format "markdown"

@test "Worker node failure and recovery"
@test "Load balancing with heterogeneous worker capabilities"
@test "Priority-based task scheduling"
@test "Deadline satisfaction rate > 99.9%"
```