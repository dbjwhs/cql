// MIT License
// Copyright (c) 2025 dbjwhs

/**
 * @file logger_examples.cpp
 * @brief Examples demonstrating the pluggable logger interface for CQL
 * 
 * This file shows various ways to integrate external logging frameworks
 * with the CQL library. Choose the approach that best fits your needs.
 */

#include <cql/logger_manager.hpp>
#include <cql/logger_adapters.hpp>
#include <iostream>
#include <memory>

// Optional: Include popular logging frameworks if available
#ifdef SPDLOG_VERSION
    #include <spdlog/spdlog.h>
    #include <spdlog/sinks/stdout_color_sinks.h>
    #include <spdlog/sinks/basic_file_sink.h>
#endif

using namespace cql;

/**
 * @brief Example 1: Use default console logger
 */
void example_default_console_logger() {
    std::cout << "\n=== Example 1: Default Console Logger ===" << std::endl;
    
    // Initialize with default console logger
    LoggerManager::initialize(LogLevel::DEBUG, true); // DEBUG level, colored output
    
    // Use the logger
    LoggerManager::log_info("CQL initialized with default console logger");
    LoggerManager::log_debug("This is a debug message");
    LoggerManager::log_error("This is an error message");
    
    LoggerManager::shutdown();
}

/**
 * @brief Example 2: Custom logger implementation
 */
class CustomBusinessLogger : public LoggerInterface {
private:
    std::string m_app_name;
    LogLevel m_min_level;

public:
    explicit CustomBusinessLogger(const std::string& app_name, LogLevel min_level = LogLevel::DEBUG)
        : m_app_name(app_name), m_min_level(min_level) {}
    
    void log(LogLevel level, const std::string& message) override {
        if (!is_level_enabled(level)) return;
        
        std::string prefix = "[" + m_app_name + "] " + log_level_to_string(level) + ": ";
        
        if (level >= LogLevel::ERROR) {
            std::cerr << prefix << message << std::endl;
        } else {
            std::cout << prefix << message << std::endl;
        }
    }
    
    bool is_level_enabled(LogLevel level) const override {
        return level >= m_min_level;
    }
    
    void flush() override {
        std::cout.flush();
        std::cerr.flush();
    }
};

void example_custom_logger() {
    std::cout << "\n=== Example 2: Custom Logger Implementation ===" << std::endl;
    
    // Create and initialize custom logger
    auto custom_logger = std::make_unique<CustomBusinessLogger>("MyApp", LogLevel::INFO);
    LoggerManager::initialize(std::move(custom_logger));
    
    LoggerManager::log_debug("This debug message will be filtered out");
    LoggerManager::log_info("Application started successfully");
    LoggerManager::log_error("A sample error occurred");
    
    LoggerManager::shutdown();
}

/**
 * @brief Example 3: Callback-based logger
 */
void example_callback_logger() {
    std::cout << "\n=== Example 3: Callback-Based Logger ===" << std::endl;
    
    // Simple callback that forwards to existing logging system
    auto logging_callback = [](LogLevel level, const char* message) {
        std::string prefix = "[CALLBACK] " + log_level_to_string(level) + ": ";
        std::cout << prefix << message << std::endl;
    };
    
    // Optional level filter
    auto level_filter = [](LogLevel level) {
        return level >= LogLevel::INFO; // Only INFO and above
    };
    
    LoggerManager::initialize_with_callback(logging_callback, level_filter);
    
    LoggerManager::log_debug("Filtered debug message");
    LoggerManager::log_info("Info message via callback");
    LoggerManager::log_critical("Critical message via callback");
    
    LoggerManager::shutdown();
}

/**
 * @brief Example 4: File-based logger
 */
void example_file_logger() {
    std::cout << "\n=== Example 4: File-Based Logger ===" << std::endl;
    
    try {
        // Create file logger
        auto file_logger = std::make_unique<adapters::FileLogger>("cql_example.log", true);
        file_logger->set_min_level(LogLevel::INFO);
        file_logger->set_auto_flush(true);
        
        LoggerManager::initialize(std::move(file_logger));
        
        LoggerManager::log_info("This message goes to cql_example.log");
        LoggerManager::log_error("Error message also goes to file");
        
        std::cout << "Messages logged to cql_example.log" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "File logger error: " << e.what() << std::endl;
    }
    
    LoggerManager::shutdown();
}

/**
 * @brief Example 5: Multi-logger (console + file)
 */
void example_multi_logger() {
    std::cout << "\n=== Example 5: Multi-Logger (Console + File) ===" << std::endl;
    
    try {
        auto multi_logger = std::make_unique<adapters::MultiLogger>();
        
        // Add console logger
        auto console_logger = std::make_unique<DefaultConsoleLogger>();
        console_logger->set_min_level(LogLevel::INFO);
        multi_logger->add_logger(std::move(console_logger));
        
        // Add file logger
        auto file_logger = std::make_unique<adapters::FileLogger>("multi_example.log");
        file_logger->set_min_level(LogLevel::DEBUG);
        multi_logger->add_logger(std::move(file_logger));
        
        LoggerManager::initialize(std::move(multi_logger));
        
        LoggerManager::log_debug("Debug: Only in file (console filters to INFO+)");
        LoggerManager::log_info("Info: Both console and file");
        LoggerManager::log_error("Error: Both console and file");
        
        std::cout << "Messages logged to both console and multi_example.log" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Multi-logger error: " << e.what() << std::endl;
    }
    
    LoggerManager::shutdown();
}

/**
 * @brief Example 6: Async logger for high-performance scenarios
 */
void example_async_logger() {
    std::cout << "\n=== Example 6: Async Logger ===" << std::endl;
    
    try {
        // Create underlying logger
        auto file_logger = std::make_unique<adapters::FileLogger>("async_example.log");
        file_logger->set_auto_flush(false); // Let async logger handle flushing
        
        // Wrap in async logger
        auto async_logger = std::make_unique<adapters::AsyncLogger>(std::move(file_logger), 1000);
        
        LoggerManager::initialize(std::move(async_logger));
        
        // Log many messages quickly
        for (int i = 0; i < 100; ++i) {
            LoggerManager::log_info("High-throughput message #" + std::to_string(i));
        }
        
        std::cout << "100 messages logged asynchronously to async_example.log" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Async logger error: " << e.what() << std::endl;
    }
    
    LoggerManager::shutdown();
}

#ifdef SPDLOG_VERSION
/**
 * @brief Example 7: Integration with spdlog
 */
void example_spdlog_integration() {
    std::cout << "\n=== Example 7: spdlog Integration ===" << std::endl;
    
    try {
        // Create spdlog logger with multiple sinks
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("spdlog_example.log", true));
        
        auto spdlog_logger = std::make_shared<spdlog::logger>("cql", sinks.begin(), sinks.end());
        spdlog_logger->set_level(spdlog::level::debug);
        spdlog_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
        
        // Create CQL adapter
        auto adapter = std::make_unique<adapters::SpdlogAdapter>(spdlog_logger);
        LoggerManager::initialize(std::move(adapter));
        
        LoggerManager::log_debug("spdlog debug message");
        LoggerManager::log_info("spdlog info message");
        LoggerManager::log_error("spdlog error message");
        
        std::cout << "Messages logged via spdlog to console and spdlog_example.log" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "spdlog integration error: " << e.what() << std::endl;
    }
    
    LoggerManager::shutdown();
}
#endif

/**
 * @brief Example 8: Temporary logger for specific operations
 */
void example_temporary_logger() {
    std::cout << "\n=== Example 8: Temporary Logger ===" << std::endl;
    
    // Set up main logger
    LoggerManager::initialize(LogLevel::INFO);
    LoggerManager::log_info("Using main logger");
    
    {
        // Create temporary logger for specific operation
        auto temp_logger = std::make_unique<CustomBusinessLogger>("TempOperation", LogLevel::DEBUG);
        TemporaryLogger temp_scope(std::move(temp_logger));
        
        LoggerManager::log_debug("Debug message in temporary logger");
        LoggerManager::log_info("Info message in temporary logger");
        
    } // Temporary logger automatically restored here
    
    LoggerManager::log_info("Back to main logger");
    
    LoggerManager::shutdown();
}

/**
 * @brief Example 9: Disable all logging (production optimization)
 */
void example_null_logger() {
    std::cout << "\n=== Example 9: Null Logger (Disabled Logging) ===" << std::endl;
    
    LoggerManager::initialize_null();
    
    // These messages will be completely ignored (no overhead)
    LoggerManager::log_debug("This debug message is ignored");
    LoggerManager::log_info("This info message is ignored");
    LoggerManager::log_error("Even this error message is ignored");
    
    std::cout << "All logging disabled - no output from CQL" << std::endl;
    
    LoggerManager::shutdown();
}

/**
 * @brief Example 10: Using convenience macros
 */
void example_convenience_macros() {
    std::cout << "\n=== Example 10: Convenience Macros ===" << std::endl;
    
    LoggerManager::initialize(LogLevel::DEBUG);
    
    // Basic logging macros
    CQL_LOG_DEBUG("Debug message via macro");
    CQL_LOG_INFO("Info message via macro");
    CQL_LOG_NORMAL("Normal message via macro");
    CQL_LOG_ERROR("Error message via macro");
    CQL_LOG_CRITICAL("Critical message via macro");
    
    // Conditional logging macros
    bool error_occurred = true;
    bool debug_enabled = false;
    
    CQL_LOG_ERROR_IF(error_occurred, "Conditional error: something went wrong");
    CQL_LOG_DEBUG_IF(debug_enabled, "This debug message won't appear");
    
    LoggerManager::shutdown();
}

/**
 * @brief Main function demonstrating all examples
 */
int main() {
    std::cout << "CQL Pluggable Logger Examples" << std::endl;
    std::cout << "=============================" << std::endl;
    
    try {
        example_default_console_logger();
        example_custom_logger();
        example_callback_logger();
        example_file_logger();
        example_multi_logger();
        example_async_logger();
        
#ifdef SPDLOG_VERSION
        example_spdlog_integration();
#else
        std::cout << "\n=== Example 7: spdlog Integration (SKIPPED - spdlog not available) ===" << std::endl;
#endif
        
        example_temporary_logger();
        example_null_logger();
        example_convenience_macros();
        
        std::cout << "\n=== All Examples Completed ===" << std::endl;
        std::cout << "Check the generated log files: cql_example.log, multi_example.log, async_example.log";
        
#ifdef SPDLOG_VERSION
        std::cout << ", spdlog_example.log";
#endif
        
        std::cout << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Example error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
