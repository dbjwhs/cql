// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <memory>
#include <functional>

namespace cql {

/**
 * @brief Log levels supported by the CQL logging system
 */
enum class LogLevel {
    DEBUG = 0,    // Detailed information for diagnosing problems
    INFO = 1,     // General information about system operation
    NORMAL = 2,   // Normal operational messages
    ERROR = 3,    // Error conditions that don't halt execution
    CRITICAL = 4  // Critical errors that may halt execution
};

/**
 * @brief Convert LogLevel to string representation
 */
inline std::string log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO"; 
        case LogLevel::NORMAL: return "NORMAL";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Abstract interface for pluggable loggers
 * 
 * This interface allows external users to integrate their own logging systems
 * (spdlog, glog, custom loggers, etc.) with CQL. Users can implement this
 * interface to bridge CQL logging to their preferred logging framework.
 */
class LoggerInterface {
public:
    virtual ~LoggerInterface() = default;
    
    /**
     * @brief Log a message at the specified level
     * @param level The log level for this message
     * @param message The formatted log message
     */
    virtual void log(LogLevel level, const std::string& message) = 0;
    
    /**
     * @brief Check if a log level is enabled
     * @param level The log level to check
     * @return true if the level is enabled, false otherwise
     */
    virtual bool is_level_enabled(LogLevel level) const = 0;
    
    /**
     * @brief Flush any buffered log messages
     */
    virtual void flush() = 0;
};

/**
 * @brief Function pointer type for simple logging callbacks
 * 
 * For users who prefer function pointers over interface implementation.
 * Signature: void(LogLevel level, const char* message)
 */
using LoggingCallback = std::function<void(LogLevel, const char*)>;

/**
 * @brief Level filtering function pointer type
 * 
 * Allows users to provide custom level filtering logic.
 * Signature: bool(LogLevel level)
 */
using LevelFilter = std::function<bool(LogLevel)>;

/**
 * @brief Default console logger implementation
 * 
 * Provides a simple console-based logger that can be used as a fallback
 * or for basic logging needs. Outputs to stdout for normal messages
 * and stderr for errors/critical messages.
 */
class DefaultConsoleLogger : public LoggerInterface {
public:
    DefaultConsoleLogger();
    ~DefaultConsoleLogger() override = default;
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;
    
    /**
     * @brief Set the minimum log level to display
     * @param min_level Messages below this level will be filtered out
     */
    void set_min_level(LogLevel min_level);
    
    /**
     * @brief Enable or disable colored output (if supported by terminal)
     * @param enable true to enable colors, false to disable
     */
    void set_colored_output(bool enable);
    
private:
    LogLevel m_min_level{LogLevel::DEBUG};
    bool m_colored_output{true};
    
    std::string format_message(LogLevel level, const std::string& message) const;
    std::string get_color_code(LogLevel level) const;
    static std::string get_timestamp();
};

/**
 * @brief Null logger implementation that discards all log messages
 * 
 * Useful for performance-critical scenarios where logging should be
 * completely disabled.
 */
class NullLogger : public LoggerInterface {
public:
    void log(LogLevel /*level*/, const std::string& /*message*/) override {}
    bool is_level_enabled(LogLevel /*level*/) const override { return false; }
    void flush() override {}
};

/**
 * @brief Callback-based logger implementation
 * 
 * Allows users to provide a simple callback function instead of implementing
 * the full interface. Useful for quick integration with existing logging systems.
 */
class CallbackLogger : public LoggerInterface {
public:
    /**
     * @brief Constructor with logging callback
     * @param callback Function to call for each log message
     * @param level_filter Optional function to filter log levels
     */
    explicit CallbackLogger(LoggingCallback callback, 
                           LevelFilter level_filter = nullptr);
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;
    
private:
    LoggingCallback m_callback;
    LevelFilter m_level_filter;
};

} // namespace cql
