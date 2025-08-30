// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "logger_interface.hpp"
#include <memory>
#include <mutex>
#include <atomic>
#include <optional>
#include <sstream>

namespace cql {

/**
 * @brief Central logger manager for the CQL library
 * 
 * This class provides a singleton interface for managing logging throughout
 * the CQL library. External users can plug in their own logger implementations
 * during library initialization. The manager provides thread-safe access to
 * the configured logger and ensures consistent logging behavior across all
 * CQL components.
 * 
 * Usage for library users:
 * @code
 * // Option 1: Use default console logger
 * LoggerManager::initialize();
 * 
 * // Option 2: Use custom logger implementation
 * auto custom_logger = std::make_unique<MyCustomLogger>();
 * LoggerManager::initialize(std::move(custom_logger));
 * 
 * // Option 3: Use callback-based logger
 * LoggerManager::initialize_with_callback([](LogLevel level, const char* msg) {
 *     my_logging_framework->log(convert_level(level), msg);
 * });
 * 
 * // Option 4: Disable all logging
 * LoggerManager::initialize_null();
 * @endcode
 */
class LoggerManager {
    friend class TemporaryLogger;
public:
    /**
     * @brief Initialize with default console logger
     * @param min_level Minimum log level to display
     * @param colored_output Whether to use colored output (auto-detected if not specified)
     */
    static void initialize(LogLevel min_level = LogLevel::DEBUG, 
                          std::optional<bool> colored_output = std::nullopt);
    
    /**
     * @brief Initialize with a custom logger implementation
     * @param logger Custom logger instance (takes ownership)
     */
    static void initialize(std::unique_ptr<LoggerInterface> logger);
    
    /**
     * @brief Initialize with a callback function
     * @param callback Function to call for each log message
     * @param level_filter Optional function to filter log levels
     */
    static void initialize_with_callback(LoggingCallback callback,
                                       LevelFilter level_filter = nullptr);
    
    /**
     * @brief Initialize with null logger (disables all logging)
     */
    static void initialize_null();
    
    /**
     * @brief Check if the logger has been initialized
     * @return true if initialized, false otherwise
     */
    static bool is_initialized();
    
    /**
     * @brief Get the current logger instance
     * @return Reference to the current logger
     * @throws std::runtime_error if not initialized
     */
    static LoggerInterface& get_logger();
    
    /**
     * @brief Shutdown the logger system
     * 
     * This should be called during library cleanup. After shutdown,
     * any logging calls will use a fallback console logger.
     */
    static void shutdown();
    
    /**
     * @brief Convenience method to log a message
     * @param level Log level
     * @param message Message to log
     */
    static void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Convenience method to check if a level is enabled
     * @param level Log level to check
     * @return true if enabled, false otherwise
     */
    static bool is_level_enabled(LogLevel level);
    
    /**
     * @brief Flush the current logger
     */
    static void flush();

    // Variadic template convenience methods for formatted logging
    template<typename... Args>
    static void log_debug(const Args&... args) {
        log_formatted(LogLevel::DEBUG, args...);
    }
    
    template<typename... Args>
    static void log_info(const Args&... args) {
        log_formatted(LogLevel::INFO, args...);
    }
    
    template<typename... Args>
    static void log_normal(const Args&... args) {
        log_formatted(LogLevel::NORMAL, args...);
    }
    
    template<typename... Args>
    static void log_error(const Args&... args) {
        log_formatted(LogLevel::ERROR, args...);
    }
    
    template<typename... Args>
    static void log_critical(const Args&... args) {
        log_formatted(LogLevel::CRITICAL, args...);
    }

private:
    static std::unique_ptr<LoggerInterface> s_logger;
    static std::mutex s_logger_mutex;
    static std::atomic<bool> s_initialized;
    static std::unique_ptr<DefaultConsoleLogger> s_fallback_logger;
    
    // Initialize fallback logger for use when main logger is not available
    static void ensure_fallback_logger();
    
    // Helper method for formatted logging
    template<typename... Args>
    static void log_formatted(LogLevel level, const Args&... args) {
        if (!is_level_enabled(level)) {
            return;
        }
        
        std::ostringstream oss;
        (oss << ... << args);
        log(level, oss.str());
    }
    
    // Prevent instantiation
    LoggerManager() = delete;
    ~LoggerManager() = delete;
    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;
};

/**
 * @brief RAII helper for temporary logger configuration
 * 
 * Allows temporarily switching to a different logger for a specific scope,
 * then automatically restoring the previous logger when destroyed.
 */
class TemporaryLogger {
public:
    /**
     * @brief Constructor that switches to a temporary logger
     * @param temp_logger Temporary logger to use
     */
    explicit TemporaryLogger(std::unique_ptr<LoggerInterface> temp_logger);
    
    /**
     * @brief Destructor that restores the previous logger
     */
    ~TemporaryLogger();
    
    // Non-copyable and non-movable
    TemporaryLogger(const TemporaryLogger&) = delete;
    TemporaryLogger& operator=(const TemporaryLogger&) = delete;
    TemporaryLogger(TemporaryLogger&&) = delete;
    TemporaryLogger& operator=(TemporaryLogger&&) = delete;

private:
    std::unique_ptr<LoggerInterface> m_previous_logger;
    bool m_had_previous_logger;
};

} // namespace cql

// Convenience macros for logging (optional, for users who prefer macros)
#define CQL_LOG_DEBUG(...) ::cql::LoggerManager::log_debug(__VA_ARGS__)
#define CQL_LOG_INFO(...) ::cql::LoggerManager::log_info(__VA_ARGS__)
#define CQL_LOG_NORMAL(...) ::cql::LoggerManager::log_normal(__VA_ARGS__)
#define CQL_LOG_ERROR(...) ::cql::LoggerManager::log_error(__VA_ARGS__)
#define CQL_LOG_CRITICAL(...) ::cql::LoggerManager::log_critical(__VA_ARGS__)

// Conditional logging macros (only log if level is enabled)
#define CQL_LOG_DEBUG_IF(condition, ...) do { if (condition) CQL_LOG_DEBUG(__VA_ARGS__); } while(0)
#define CQL_LOG_INFO_IF(condition, ...) do { if (condition) CQL_LOG_INFO(__VA_ARGS__); } while(0)
#define CQL_LOG_NORMAL_IF(condition, ...) do { if (condition) CQL_LOG_NORMAL(__VA_ARGS__); } while(0)
#define CQL_LOG_ERROR_IF(condition, ...) do { if (condition) CQL_LOG_ERROR(__VA_ARGS__); } while(0)
#define CQL_LOG_CRITICAL_IF(condition, ...) do { if (condition) CQL_LOG_CRITICAL(__VA_ARGS__); } while(0)
