// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/logger_manager.hpp"
#include <stdexcept>

namespace cql {

// Static member definitions
std::unique_ptr<LoggerInterface> LoggerManager::s_logger;
std::mutex LoggerManager::s_logger_mutex;
std::atomic<bool> LoggerManager::s_initialized{false};
std::unique_ptr<DefaultConsoleLogger> LoggerManager::s_fallback_logger;

void LoggerManager::initialize(LogLevel min_level, std::optional<bool> colored_output) {
    std::lock_guard<std::mutex> lock(s_logger_mutex);
    
    auto console_logger = std::make_unique<DefaultConsoleLogger>();
    console_logger->set_min_level(min_level);
    
    if (colored_output.has_value()) {
        console_logger->set_colored_output(colored_output.value());
    }
    
    s_logger = std::move(console_logger);
    s_initialized = true;
}

void LoggerManager::initialize(std::unique_ptr<LoggerInterface> logger) {
    if (!logger) {
        throw std::invalid_argument("Logger cannot be null");
    }
    
    std::lock_guard<std::mutex> lock(s_logger_mutex);
    s_logger = std::move(logger);
    s_initialized = true;
}

void LoggerManager::initialize_with_callback(LoggingCallback callback, LevelFilter level_filter) {
    if (!callback) {
        throw std::invalid_argument("Logging callback cannot be null");
    }
    
    auto callback_logger = std::make_unique<CallbackLogger>(std::move(callback), std::move(level_filter));
    initialize(std::move(callback_logger));
}

void LoggerManager::initialize_null() {
    std::lock_guard<std::mutex> lock(s_logger_mutex);
    s_logger = std::make_unique<NullLogger>();
    s_initialized = true;
}

bool LoggerManager::is_initialized() {
    return s_initialized.load();
}

LoggerInterface& LoggerManager::get_logger() {
    if (!s_initialized.load()) {
        // Auto-initialize with default console logger if not already initialized
        initialize();
    }
    
    std::lock_guard<std::mutex> lock(s_logger_mutex);
    if (!s_logger) {
        // Fallback to console logger if somehow the logger is null
        ensure_fallback_logger();
        return *s_fallback_logger;
    }
    
    return *s_logger;
}

void LoggerManager::shutdown() {
    std::lock_guard<std::mutex> lock(s_logger_mutex);
    
    if (s_logger) {
        s_logger->flush();
        s_logger.reset();
    }
    
    s_initialized = false;
}

void LoggerManager::log(LogLevel level, const std::string& message) {
    try {
        get_logger().log(level, message);
    } catch (const std::exception& e) {
        // Fallback to console output if logger fails
        ensure_fallback_logger();
        s_fallback_logger->log(LogLevel::ERROR, 
            "Logger error: " + std::string(e.what()) + " | Original message: " + message);
    }
}

bool LoggerManager::is_level_enabled(LogLevel level) {
    try {
        return get_logger().is_level_enabled(level);
    } catch (const std::exception&) {
        // Fallback behavior - assume level is enabled
        return true;
    }
}

void LoggerManager::flush() {
    try {
        get_logger().flush();
    } catch (const std::exception&) {
        // Ignore flush errors - not critical
    }
}

void LoggerManager::ensure_fallback_logger() {
    if (!s_fallback_logger) {
        s_fallback_logger = std::make_unique<DefaultConsoleLogger>();
        s_fallback_logger->set_min_level(LogLevel::DEBUG);
    }
}

// TemporaryLogger implementation
TemporaryLogger::TemporaryLogger(std::unique_ptr<LoggerInterface> temp_logger) {
    std::lock_guard<std::mutex> lock(LoggerManager::s_logger_mutex);
    
    // Save current logger state
    m_had_previous_logger = LoggerManager::s_initialized.load();
    if (m_had_previous_logger) {
        m_previous_logger = std::move(LoggerManager::s_logger);
    }
    
    // Set temporary logger
    LoggerManager::s_logger = std::move(temp_logger);
    LoggerManager::s_initialized = true;
}

TemporaryLogger::~TemporaryLogger() {
    std::lock_guard<std::mutex> lock(LoggerManager::s_logger_mutex);
    
    // Flush current logger before switching
    if (LoggerManager::s_logger) {
        try {
            LoggerManager::s_logger->flush();
        } catch (const std::exception&) {
            // Ignore flush errors during destruction
        }
    }
    
    // Restore previous logger state
    if (m_had_previous_logger) {
        LoggerManager::s_logger = std::move(m_previous_logger);
        LoggerManager::s_initialized = true;
    } else {
        LoggerManager::s_logger.reset();
        LoggerManager::s_initialized = false;
    }
}

} // namespace cql
