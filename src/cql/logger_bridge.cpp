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
    
    // Update local cache for thread safety
    size_t index = historic_level_to_index(level);
    m_level_enabled_cache[index] = enabled;
    
    // For thread safety, we only update the local cache
    // The underlying bridge state is not synchronized to avoid dynamic_cast crashes
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
    // Return cached state for thread safety
    size_t index = historic_level_to_index(level);
    return m_level_enabled_cache[index].load();
}

void LoggerBridge::disableStderr() {
    ensure_logger_manager_initialized();
    
    // Update local cache for thread safety
    m_stderr_enabled_cache = false;
    
    // For thread safety, we only update the local cache
    // The underlying bridge state is not synchronized to avoid dynamic_cast crashes
}

void LoggerBridge::enableStderr() {
    ensure_logger_manager_initialized();
    
    // Update local cache for thread safety
    m_stderr_enabled_cache = true;
    
    // For thread safety, we only update the local cache  
    // The underlying bridge state is not synchronized to avoid dynamic_cast crashes
}

bool LoggerBridge::isStderrEnabled() const {
    // Return the cached state for thread safety
    // The actual state is also tracked in HistoricLoggerBridge but
    // accessing it via dynamic_cast in concurrent scenarios can cause crashes
    return m_stderr_enabled_cache.load();
}

void LoggerBridge::setFileOutputEnabled(bool enabled) {
    ensure_logger_manager_initialized();
    
    // For thread safety, this is a no-op
    // Historic Logger behavior is maintained via isFileOutputEnabled() returning true
    (void)enabled; // Suppress unused parameter warning
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
    // Note: We don't catch exceptions here as get_logger() shouldn't throw
    LoggerInterface& current_logger = LoggerManager::get_logger();
    return dynamic_cast<HistoricLoggerBridge*>(&current_logger);
}

size_t LoggerBridge::historic_level_to_index(HistoricLogLevel level) {
    // Map HistoricLogLevel to cache array index
    // Historic: INFO=0, NORMAL=1, DEBUG=2, ERROR=3, CRITICAL=4
    switch (level) {
        case HistoricLogLevel::INFO:     return 0;
        case HistoricLogLevel::NORMAL:   return 1;
        case HistoricLogLevel::DEBUG:    return 2;
        case HistoricLogLevel::ERROR:    return 3;
        case HistoricLogLevel::CRITICAL: return 4;
        default:                         return 0; // Default to INFO
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
