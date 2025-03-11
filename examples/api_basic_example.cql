@copyright "MIT License" "2025 CQL Project Contributors"
@language "C++"
@description "implement a simple logger class with multiple log levels"

@context "Modern C++20 implementation"
@context "Thread-safe implementation"
@dependency "std::mutex, std::ofstream"

@performance "Low overhead for disabled log levels"
@security "Safe file handling"

@test "Test all log levels"
@test "Test thread safety with concurrent logging"
@test "Test filtering by log level"