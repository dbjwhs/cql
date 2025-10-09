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
FileLogger::FileLogger(const std::string& file_path, bool append)
    : m_file_path(file_path) {
    auto open_mode = append ? (std::ios::out | std::ios::app) : std::ios::out;
    m_file.open(file_path, open_mode);

    if (!m_file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + file_path);
    }

    // Get initial file size if appending
    if (append) {
        m_file.seekp(0, std::ios::end);
        auto pos = m_file.tellp();
        if (pos < 0) {
            // tellp() failed, start with 0
            m_current_file_size = 0;
        } else {
            m_current_file_size = static_cast<size_t>(pos);
        }
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
    size_t message_size = formatted_message.length() + 1; // +1 for newline

    // Check if rotation is needed before writing
    if (m_rotation_enabled && m_current_file_size + message_size >= m_max_file_size) {
        perform_rotation();
        // m_current_file_size is now 0 after rotation
    }

    m_file << formatted_message << std::endl;

    // Update size tracker
    if (m_rotation_enabled) {
        m_current_file_size += message_size;
    }

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

void FileLogger::enable_rotation(size_t max_size_bytes, size_t max_files) {
    std::lock_guard<std::mutex> lock(m_file_mutex);
    m_rotation_enabled = true;
    m_max_file_size = max_size_bytes;
    m_max_files = max_files;
}

void FileLogger::disable_rotation() {
    std::lock_guard<std::mutex> lock(m_file_mutex);
    m_rotation_enabled = false;
}

void FileLogger::set_timestamp_format(TimestampFormat format) {
    std::lock_guard<std::mutex> lock(m_file_mutex);
    m_timestamp_format = format;
}

size_t FileLogger::get_current_file_size() const {
    std::lock_guard<std::mutex> lock(m_file_mutex);
    return m_current_file_size;
}

void FileLogger::perform_rotation() {
    // Flush and close current file
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }

    // Rotate existing files: file.log.4 -> delete, file.log.3 -> file.log.4, etc.
    // Only do this if we have a file limit (max_files > 0)
    if (m_max_files > 0) {
        for (int i = static_cast<int>(m_max_files) - 1; i >= 1; --i) {
            std::string old_name = m_file_path + "." + std::to_string(i);
            std::string new_name = m_file_path + "." + std::to_string(i + 1);

            // Remove oldest file if it exists (use filesystem::remove for better error handling)
            if (i == static_cast<int>(m_max_files) - 1) {
                std::error_code ec;
                std::filesystem::remove(new_name, ec);
                // Ignore errors - file may not exist
            }

            // Rename file.log.N to file.log.N+1 (only if old file exists)
            if (std::filesystem::exists(old_name)) {
                // Ensure target doesn't exist before rename
                std::error_code ec;
                std::filesystem::remove(new_name, ec);

                if (std::rename(old_name.c_str(), new_name.c_str()) != 0) {
                    // Log error but continue rotation - this is not fatal
                    std::cerr << "Warning: Failed to rename " << old_name << " to " << new_name << std::endl;
                }
            }
        }
    } else {
        // Unlimited rotation: rename existing files to higher numbers
        // Find the highest numbered file
        int highest = 0;
        for (int i = 1; i <= 1000; ++i) {  // Limit search to prevent infinite loop
            if (std::filesystem::exists(m_file_path + "." + std::to_string(i))) {
                highest = i;
            } else {
                break;
            }
        }

        // Rename files from highest to lowest
        for (int i = highest; i >= 1; --i) {
            std::string old_name = m_file_path + "." + std::to_string(i);
            std::string new_name = m_file_path + "." + std::to_string(i + 1);
            if (std::filesystem::exists(old_name)) {
                std::rename(old_name.c_str(), new_name.c_str());
            }
        }
    }

    // Rename current file to file.log.1
    std::string first_rotated = m_file_path + ".1";

    // Ensure .1 doesn't exist before attempting rename
    std::error_code remove_ec;
    std::filesystem::remove(first_rotated, remove_ec);

    if (std::rename(m_file_path.c_str(), first_rotated.c_str()) != 0) {
        // This is more serious - try to reopen the original file
        std::cerr << "Warning: Failed to rename " << m_file_path << " to " << first_rotated << std::endl;
        m_file.open(m_file_path, std::ios::out | std::ios::app);
        if (!m_file.is_open()) {
            throw std::runtime_error("Failed to reopen log file after failed rotation: " + m_file_path);
        }
        return; // Abort rotation but keep logging to original file
    }

    // Open new file
    m_file.open(m_file_path, std::ios::out);
    if (!m_file.is_open()) {
        throw std::runtime_error("Failed to open rotated log file: " + m_file_path);
    }

    m_current_file_size = 0;
}

std::string FileLogger::format_message(LogLevel level, const std::string& message) const {
    std::ostringstream formatted;

    // Add timestamp if enabled
    if (m_timestamp_format != TimestampFormat::NONE) {
        formatted << get_timestamp() << " ";
    }

    formatted << "[" << log_level_to_string(level) << "]"
              << " [Thread:" << std::this_thread::get_id() << "] "
              << message;
    return formatted.str();
}

std::string FileLogger::get_timestamp() const {
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto time = std::chrono::system_clock::to_time_t(now);

    struct tm tm_buf{};

    switch (m_timestamp_format) {
        case TimestampFormat::ISO8601: {
            // UTC: 2025-10-07T14:30:45.123Z
#ifdef _WIN32
            gmtime_s(&tm_buf, &time);
#else
            gmtime_r(&time, &tm_buf);
#endif
            std::ostringstream ss;
            ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
            return ss.str();
        }

        case TimestampFormat::ISO8601_LOCAL: {
            // Local time with timezone: 2025-10-07T14:30:45.123-0700
#ifdef _WIN32
            localtime_s(&tm_buf, &time);
#else
            localtime_r(&time, &tm_buf);
#endif
            std::ostringstream ss;
            ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            ss << std::put_time(&tm_buf, "%z");
            return ss.str();
        }

        case TimestampFormat::SIMPLE: {
            // Simple format: 2025-10-07 14:30:45.123
#ifdef _WIN32
            gmtime_s(&tm_buf, &time);
#else
            gmtime_r(&time, &tm_buf);
#endif
            std::ostringstream ss;
            ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return ss.str();
        }

        case TimestampFormat::EPOCH_MS: {
            // Milliseconds since epoch
            auto epoch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            return std::to_string(epoch_ms);
        }

        case TimestampFormat::NONE:
        default:
            return "";
    }
}

// LevelFilteredLogger implementation
LevelFilteredLogger::LevelFilteredLogger(std::unique_ptr<LoggerInterface> logger,
                                         LogLevel min_level)
    : m_logger(std::move(logger)), m_min_level(min_level) {
    if (!m_logger) {
        throw std::invalid_argument("Logger cannot be null");
    }
}

void LevelFilteredLogger::log(LogLevel level, const std::string& message) {
    if (is_level_enabled(level)) {
        m_logger->log(level, message);
    }
}

bool LevelFilteredLogger::is_level_enabled(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(m_min_level.load());
}

void LevelFilteredLogger::flush() {
    m_logger->flush();
}

void LevelFilteredLogger::set_min_level(LogLevel min_level) {
    m_min_level.store(min_level);
}

LogLevel LevelFilteredLogger::get_min_level() const {
    return m_min_level.load();
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
