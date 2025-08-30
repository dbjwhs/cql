// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/logger_adapters.hpp"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <queue>
#include <condition_variable>

// Optional framework includes (only if available)
#ifdef SPDLOG_VERSION
    #include <spdlog/spdlog.h>
#endif

#ifdef GLOG_VERSION
    #include <glog/logging.h>
#endif

#ifdef BOOST_LOG_VERSION
    #include <boost/log/trivial.hpp>
#endif

namespace cql {
namespace adapters {

#ifdef SPDLOG_VERSION
SpdlogAdapter::SpdlogAdapter(std::shared_ptr<spdlog::logger> logger)
    : m_logger(std::move(logger)) {
    if (!m_logger) {
        throw std::invalid_argument("spdlog logger cannot be null");
    }
}

void SpdlogAdapter::log(LogLevel level, const std::string& message) {
    if (!is_level_enabled(level)) {
        return;
    }
    
    m_logger->log(convert_level(level), message);
}

bool SpdlogAdapter::is_level_enabled(LogLevel level) const {
    return m_logger->should_log(convert_level(level));
}

void SpdlogAdapter::flush() {
    m_logger->flush();
}

spdlog::level::level_enum SpdlogAdapter::convert_level(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return spdlog::level::debug;
        case LogLevel::INFO:     return spdlog::level::info;
        case LogLevel::NORMAL:   return spdlog::level::info;
        case LogLevel::ERROR:    return spdlog::level::err;
        case LogLevel::CRITICAL: return spdlog::level::critical;
        default:                 return spdlog::level::info;
    }
}
#endif // SPDLOG_VERSION

#ifdef GLOG_VERSION
void GlogAdapter::log(LogLevel level, const std::string& message) {
    if (!is_level_enabled(level)) {
        return;
    }
    
    int glog_level = convert_level(level);
    
    // Use Google Log's LOG macro equivalents
    switch (level) {
        case LogLevel::DEBUG:
        case LogLevel::INFO:
        case LogLevel::NORMAL:
            LOG(INFO) << message;
            break;
        case LogLevel::ERROR:
            LOG(ERROR) << message;
            break;
        case LogLevel::CRITICAL:
            LOG(FATAL) << message;
            break;
    }
}

bool GlogAdapter::is_level_enabled(LogLevel level) const {
    // glog doesn't have a direct "is enabled" check, so we assume all levels are enabled
    // Users can control this via glog's own configuration
    return true;
}

void GlogAdapter::flush() {
    google::FlushLogFiles(google::GLOG_INFO);
}

int GlogAdapter::convert_level(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return google::GLOG_INFO;
        case LogLevel::INFO:     return google::GLOG_INFO;
        case LogLevel::NORMAL:   return google::GLOG_INFO;
        case LogLevel::ERROR:    return google::GLOG_ERROR;
        case LogLevel::CRITICAL: return google::GLOG_FATAL;
        default:                 return google::GLOG_INFO;
    }
}
#endif // GLOG_VERSION

#ifdef BOOST_LOG_VERSION
void BoostLogAdapter::log(LogLevel level, const std::string& message) {
    if (!is_level_enabled(level)) {
        return;
    }
    
    BOOST_LOG_TRIVIAL(convert_level(level)) << message;
}

bool BoostLogAdapter::is_level_enabled(LogLevel level) const {
    // For simplicity, assume all levels are enabled
    // Users can configure Boost.Log filters separately
    return true;
}

void BoostLogAdapter::flush() {
    // Boost.Log doesn't have a direct flush method
    // Flushing is typically handled automatically
}

boost::log::trivial::severity_level BoostLogAdapter::convert_level(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return boost::log::trivial::debug;
        case LogLevel::INFO:     return boost::log::trivial::info;
        case LogLevel::NORMAL:   return boost::log::trivial::info;
        case LogLevel::ERROR:    return boost::log::trivial::error;
        case LogLevel::CRITICAL: return boost::log::trivial::fatal;
        default:                 return boost::log::trivial::info;
    }
}
#endif // BOOST_LOG_VERSION

// FileLogger implementation
FileLogger::FileLogger(const std::string& file_path, bool append) {
    auto open_mode = append ? (std::ios::out | std::ios::app) : std::ios::out;
    m_file.open(file_path, open_mode);
    
    if (!m_file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + file_path);
    }
}

FileLogger::~FileLogger() {
    if (m_file.is_open()) {
        flush();
        m_file.close();
    }
}

void FileLogger::log(LogLevel level, const std::string& message) {
    if (!is_level_enabled(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_file_mutex);
    std::string formatted_message = format_message(level, message);
    m_file << formatted_message << std::endl;
    
    if (m_auto_flush) {
        m_file.flush();
    }
}

bool FileLogger::is_level_enabled(LogLevel level) const {
    return level >= m_min_level;
}

void FileLogger::flush() {
    std::lock_guard<std::mutex> lock(m_file_mutex);
    m_file.flush();
}

void FileLogger::set_min_level(LogLevel min_level) {
    m_min_level = min_level;
}

void FileLogger::set_auto_flush(bool enable) {
    m_auto_flush = enable;
}

std::string FileLogger::format_message(LogLevel level, const std::string& message) const {
    std::ostringstream formatted;
    formatted << get_timestamp()
              << " [" << log_level_to_string(level) << "]"
              << " [Thread:" << std::this_thread::get_id() << "] "
              << message;
    return formatted.str();
}

std::string FileLogger::get_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto time = std::chrono::system_clock::to_time_t(now);
    
    struct tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &time);
#else
    gmtime_r(&time, &tm_buf);
#endif
    
    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << " UTC";
    return ss.str();
}

// MultiLogger implementation
void MultiLogger::add_logger(std::unique_ptr<LoggerInterface> logger) {
    if (!logger) {
        throw std::invalid_argument("Logger cannot be null");
    }
    
    std::lock_guard<std::mutex> lock(m_loggers_mutex);
    m_loggers.push_back(std::move(logger));
}

void MultiLogger::clear_loggers() {
    std::lock_guard<std::mutex> lock(m_loggers_mutex);
    m_loggers.clear();
}

size_t MultiLogger::logger_count() const {
    std::lock_guard<std::mutex> lock(m_loggers_mutex);
    return m_loggers.size();
}

void MultiLogger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_loggers_mutex);
    
    for (auto& logger : m_loggers) {
        try {
            logger->log(level, message);
        } catch (const std::exception&) {
            // Continue with other loggers if one fails
        }
    }
}

bool MultiLogger::is_level_enabled(LogLevel level) const {
    std::lock_guard<std::mutex> lock(m_loggers_mutex);
    
    // Return true if any logger has this level enabled
    for (const auto& logger : m_loggers) {
        try {
            if (logger->is_level_enabled(level)) {
                return true;
            }
        } catch (const std::exception&) {
            // Continue checking other loggers if one fails
        }
    }
    
    return false;
}

void MultiLogger::flush() {
    std::lock_guard<std::mutex> lock(m_loggers_mutex);
    
    for (auto& logger : m_loggers) {
        try {
            logger->flush();
        } catch (const std::exception&) {
            // Continue flushing other loggers if one fails
        }
    }
}

// AsyncLogger implementation
AsyncLogger::AsyncLogger(std::unique_ptr<LoggerInterface> logger, size_t queue_size)
    : m_logger(std::move(logger)), m_max_queue_size(queue_size) {
    
    if (!m_logger) {
        throw std::invalid_argument("Logger cannot be null");
    }
    
    // Start worker thread
    m_worker_thread = std::thread(&AsyncLogger::worker_thread_func, this);
}

AsyncLogger::~AsyncLogger() {
    // Signal shutdown
    m_shutdown = true;
    m_queue_cv.notify_all();
    
    // Wait for worker thread to finish
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
}

void AsyncLogger::log(LogLevel level, const std::string& message) {
    if (!is_level_enabled(level)) {
        return;
    }
    
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    
    // Drop messages if queue is full (non-blocking behavior)
    if (m_message_queue.size() >= m_max_queue_size) {
        return;
    }
    
    m_message_queue.push({level, message});
    lock.unlock();
    
    m_queue_cv.notify_one();
}

bool AsyncLogger::is_level_enabled(LogLevel level) const {
    return m_logger->is_level_enabled(level);
}

void AsyncLogger::flush() {
    // Wait for queue to be processed
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_queue_cv.wait(lock, [this] { return m_message_queue.empty() || m_shutdown.load(); });
    lock.unlock();
    
    // Flush the underlying logger
    if (!m_shutdown.load()) {
        m_logger->flush();
    }
}

void AsyncLogger::worker_thread_func() {
    while (!m_shutdown.load()) {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        
        // Wait for messages or shutdown signal
        m_queue_cv.wait(lock, [this] { return !m_message_queue.empty() || m_shutdown.load(); });
        
        // Process all available messages
        while (!m_message_queue.empty()) {
            auto message = std::move(m_message_queue.front());
            m_message_queue.pop();
            lock.unlock();
            
            try {
                m_logger->log(message.level, message.message);
            } catch (const std::exception&) {
                // Ignore logging errors in worker thread
            }
            
            lock.lock();
        }
        
        // Notify any threads waiting for the queue to be empty
        lock.unlock();
        m_queue_cv.notify_all();
    }
    
    // Process any remaining messages before shutdown
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    while (!m_message_queue.empty()) {
        auto message = std::move(m_message_queue.front());
        m_message_queue.pop();
        
        try {
            m_logger->log(message.level, message.message);
        } catch (const std::exception&) {
            // Ignore logging errors during shutdown
        }
    }
    
    // Final flush
    try {
        m_logger->flush();
    } catch (const std::exception&) {
        // Ignore flush errors during shutdown
    }
}

} // namespace adapters
} // namespace cql
