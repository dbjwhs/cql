// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "logger_manager.hpp"
#include "historic_logger_bridge.hpp"
#include <memory>
#include <mutex>
#include <atomic>
#include <sstream>

/**
 * @file logger_bridge.hpp
 * @brief Bridge implementation that maintains historic Logger API while using pluggable system
 * 
 * This file provides a replacement for the historic Logger class in project_utils.hpp
 * that maintains exact API compatibility while routing all calls through the new
 * pluggable logger system internally.
 */

namespace cql {

// Historic LogLevel enum for backward compatibility
enum class HistoricLogLevel {
    INFO,
    NORMAL, 
    DEBUG,
    ERROR,
    CRITICAL
};

/**
 * @brief Bridge Logger class that maintains historic API while using pluggable system
 * 
 * This class provides the exact same API as the historic Logger but routes all
 * calls through the new pluggable logger system. This allows existing CQL code
 * to work unchanged while gaining pluggable logger capabilities.
 */
class LoggerBridge {
private:
    // Singleton instance management (matching historic behavior)
    inline static std::shared_ptr<LoggerBridge> m_instance;
    inline static std::mutex m_instance_mutex;
    
    // Track whether we initialized our own historic bridge
    inline static bool m_owns_logger_manager = false;
    
    // Track stderr state locally for thread safety
    mutable std::atomic<bool> m_stderr_enabled_cache{true};
    
    // Constructor is private to control instantiation
    explicit LoggerBridge(const std::string& path);

    /**
     * @brief Convert historic LogLevel to new LogLevel
     */
    static cql::LogLevel historic_to_new_level(HistoricLogLevel historic_level);
    
    /**
     * @brief Convert new LogLevel to historic LogLevel  
     */
    static HistoricLogLevel new_to_historic_level(cql::LogLevel new_level);
    
    /**
     * @brief Ensure LoggerManager is initialized with our historic bridge
     */
    static void ensure_logger_manager_initialized(const std::string& path = "../custom.log");

public:
    /**
     * @brief RAII class for temporarily disabling stderr output
     * (Maintains exact historic API)
     */
    class StderrSuppressionGuard {
    public:
        StderrSuppressionGuard();
        ~StderrSuppressionGuard();
        
    private:
        bool m_was_enabled;
    };
    
    // Static factory methods (matching historic API exactly)
    static std::shared_ptr<LoggerBridge> getOrCreateInstance(const std::string& path = "../custom.log");
    static LoggerBridge& getInstance();
    static LoggerBridge& getInstance(const std::string& custom_path);
    static std::shared_ptr<LoggerBridge> getInstancePtr();
    static std::shared_ptr<LoggerBridge> getInstancePtr(const std::string& custom_path);
    
    // Destructor
    ~LoggerBridge() = default;
    
    // Main logging methods (variadic template matching historic API)
    template<typename... Args>
    void log(const HistoricLogLevel level, const Args&... args) {
        ensure_logger_manager_initialized();
        
        cql::LogLevel new_level = historic_to_new_level(level);
        if (!LoggerManager::is_level_enabled(new_level)) {
            return;
        }
        
        // Build message string exactly like historic logger
        std::ostringstream message_stream;
        (message_stream << ... << args);
        
        LoggerManager::log(new_level, message_stream.str());
    }
    
    // Overload to handle namespace collision with cql::LogLevel
    template<typename... Args>
    void log(const cql::LogLevel level, const Args&... args) {
        // Convert new LogLevel to historic LogLevel and delegate
        HistoricLogLevel historic_level = new_to_historic_level(level);
        log(historic_level, args...);
    }
    
    // Logging with depth (matching historic API)
    template<typename... Args>
    void log_with_depth(const HistoricLogLevel level, const int depth, const Args&... args) {
        ensure_logger_manager_initialized();
        
        cql::LogLevel new_level = historic_to_new_level(level);
        if (!LoggerManager::is_level_enabled(new_level)) {
            return;
        }
        
        // Build message with indentation like historic logger
        std::ostringstream message_stream;
        message_stream << getIndentation(depth);
        (message_stream << ... << args);
        
        LoggerManager::log(new_level, message_stream.str());
    }
    
    // Overload to handle namespace collision with cql::LogLevel
    template<typename... Args>
    void log_with_depth(const cql::LogLevel level, const int depth, const Args&... args) {
        HistoricLogLevel historic_level = new_to_historic_level(level);
        log_with_depth(historic_level, depth, args...);
    }
    
    // Level management methods (matching historic API)
    void setLevelEnabled(HistoricLogLevel level, const bool enabled);
    void setToLevelEnabled(HistoricLogLevel debug_level);
    bool isLevelEnabled(const HistoricLogLevel level) const;
    
    // Overloads to handle namespace collision with cql::LogLevel
    void setLevelEnabled(cql::LogLevel level, const bool enabled) {
        HistoricLogLevel historic_level = new_to_historic_level(level);
        setLevelEnabled(historic_level, enabled);
    }
    void setToLevelEnabled(cql::LogLevel debug_level) {
        HistoricLogLevel historic_level = new_to_historic_level(debug_level);
        setToLevelEnabled(historic_level);
    }
    bool isLevelEnabled(const cql::LogLevel level) const {
        HistoricLogLevel historic_level = new_to_historic_level(level);
        return isLevelEnabled(historic_level);
    }
    
    // Stderr control methods (matching historic API)  
    void disableStderr();
    void enableStderr();
    bool isStderrEnabled() const;
    
    // File output control methods (matching historic API)
    void setFileOutputEnabled(bool enabled);
    bool isFileOutputEnabled() const;
    
private:
    // Helper method for indentation (matching historic behavior)
    static std::string getIndentation(const int depth) {
        return std::string(static_cast<size_t>(depth * 2), ' ');
    }
    
    // Get the HistoricLoggerBridge from LoggerManager (if it exists)
    static HistoricLoggerBridge* get_historic_bridge();
};

// Convert historic LogLevel enum to new LogLevel enum
inline cql::LogLevel LoggerBridge::historic_to_new_level(HistoricLogLevel historic_level) {
    switch (historic_level) {
        case HistoricLogLevel::INFO:     return cql::LogLevel::INFO;
        case HistoricLogLevel::NORMAL:   return cql::LogLevel::NORMAL;
        case HistoricLogLevel::DEBUG:    return cql::LogLevel::DEBUG;
        case HistoricLogLevel::ERROR:    return cql::LogLevel::ERROR;
        case HistoricLogLevel::CRITICAL: return cql::LogLevel::CRITICAL;
        default:                         return cql::LogLevel::INFO;
    }
}

inline HistoricLogLevel LoggerBridge::new_to_historic_level(cql::LogLevel new_level) {
    switch (new_level) {
        case cql::LogLevel::DEBUG:    return HistoricLogLevel::DEBUG;
        case cql::LogLevel::INFO:     return HistoricLogLevel::INFO;
        case cql::LogLevel::NORMAL:   return HistoricLogLevel::NORMAL;
        case cql::LogLevel::ERROR:    return HistoricLogLevel::ERROR;
        case cql::LogLevel::CRITICAL: return HistoricLogLevel::CRITICAL;
        default:                      return HistoricLogLevel::INFO;
    }
}

} // namespace cql
