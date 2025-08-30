// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/logger_bridge.hpp"
#include "../../include/cql/logger_manager.hpp"
#include <iostream>

namespace cql {

LoggerBridge::LoggerBridge(const std::string& path) {
    // Initialize LoggerManager with HistoricLoggerBridge if not already initialized
    if (!LoggerManager::is_initialized()) {
        auto historic_bridge = std::make_unique<HistoricLoggerBridge>(path);
        LoggerManager::initialize(std::move(historic_bridge));
        m_owns_logger_manager = true;
    }
}

void LoggerBridge::ensure_logger_manager_initialized(const std::string& path) {
    if (!LoggerManager::is_initialized()) {
        auto historic_bridge = std::make_unique<HistoricLoggerBridge>(path);
        LoggerManager::initialize(std::move(historic_bridge));
        m_owns_logger_manager = true;
    }
}

std::shared_ptr<LoggerBridge> LoggerBridge::getOrCreateInstance(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_instance_mutex);
    if (!m_instance) {
        m_instance = std::shared_ptr<LoggerBridge>(new LoggerBridge(path));
    }
    return m_instance;
}

LoggerBridge& LoggerBridge::getInstance() {
    return *getOrCreateInstance();
}

LoggerBridge& LoggerBridge::getInstance(const std::string& custom_path) {
    return *getOrCreateInstance(custom_path);
}

std::shared_ptr<LoggerBridge> LoggerBridge::getInstancePtr() {
    return getOrCreateInstance();
}

std::shared_ptr<LoggerBridge> LoggerBridge::getInstancePtr(const std::string& custom_path) {
    return getOrCreateInstance(custom_path);
}

void LoggerBridge::setLevelEnabled(HistoricLogLevel level, const bool enabled) {
    ensure_logger_manager_initialized();
    
    // Try to get the HistoricLoggerBridge to configure specific level
    if (auto* bridge = get_historic_bridge()) {
        cql::LogLevel new_level = historic_to_new_level(level);
        bridge->set_level_enabled(new_level, enabled);
    }
}

void LoggerBridge::setToLevelEnabled(HistoricLogLevel debug_level) {
    ensure_logger_manager_initialized();
    
    // Configure logger to only show messages at or above the specified level
    // (matching historic behavior)
    for (int ndx = 0; ndx <= static_cast<int>(HistoricLogLevel::CRITICAL); ++ndx) {
        const auto level = static_cast<HistoricLogLevel>(ndx);
        bool should_enable = ndx >= static_cast<int>(debug_level);
        setLevelEnabled(level, should_enable);
    }
}

bool LoggerBridge::isLevelEnabled(const HistoricLogLevel level) const {
    if (!LoggerManager::is_initialized()) {
        return true; // Default behavior when not initialized
    }
    
    cql::LogLevel new_level = historic_to_new_level(level);
    return LoggerManager::is_level_enabled(new_level);
}

void LoggerBridge::disableStderr() {
    ensure_logger_manager_initialized();
    
    if (auto* bridge = get_historic_bridge()) {
        bridge->set_stderr_enabled(false);
    }
}

void LoggerBridge::enableStderr() {
    ensure_logger_manager_initialized();
    
    if (auto* bridge = get_historic_bridge()) {
        bridge->set_stderr_enabled(true);
    }
}

bool LoggerBridge::isStderrEnabled() const {
    if (!LoggerManager::is_initialized()) {
        return true; // Default behavior
    }
    
    if (auto* bridge = get_historic_bridge()) {
        return bridge->is_stderr_enabled();
    }
    return true;
}

void LoggerBridge::setFileOutputEnabled(bool enabled) {
    ensure_logger_manager_initialized();
    
    if (auto* bridge = get_historic_bridge()) {
        bridge->set_file_output_enabled(enabled);
    }
}

bool LoggerBridge::isFileOutputEnabled() const {
    // Historic Logger always had file output enabled by default
    // For backward compatibility, we maintain this behavior
    // The actual file output state is managed by the underlying HistoricLoggerBridge
    return true;
}

HistoricLoggerBridge* LoggerBridge::get_historic_bridge() {
    if (!LoggerManager::is_initialized()) {
        return nullptr;
    }
    
    // Try to dynamic_cast the current logger to HistoricLoggerBridge
    try {
        LoggerInterface& current_logger = LoggerManager::get_logger();
        return dynamic_cast<HistoricLoggerBridge*>(&current_logger);
    } catch (...) {
        return nullptr;
    }
}

// StderrSuppressionGuard implementation
LoggerBridge::StderrSuppressionGuard::StderrSuppressionGuard() 
    : m_was_enabled(LoggerBridge::getInstance().isStderrEnabled()) {
    LoggerBridge::getInstance().disableStderr();
}

LoggerBridge::StderrSuppressionGuard::~StderrSuppressionGuard() {
    if (m_was_enabled) {
        LoggerBridge::getInstance().enableStderr();
    }
}

} // namespace cql
