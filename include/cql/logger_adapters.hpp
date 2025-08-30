// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "logger_interface.hpp"
#include <memory>
#include <fstream>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <mutex>

// Forward declarations for optional external frameworks
#ifdef SPDLOG_VERSION
#include <spdlog/logger.h>
#endif

#ifdef BOOST_LOG_VERSION
#include <boost/log/trivial.hpp>
#endif

/**
 * @file logger_adapters.hpp
 * @brief Adapter classes for integrating popular logging frameworks with CQL
 * 
 * This file provides adapter implementations for common logging frameworks,
 * making it easy for users to integrate their existing logging infrastructure
 * with CQL. These adapters are optional and only compiled if the corresponding
 * logging framework is available.
 */

namespace cql {
namespace adapters {

#ifdef SPDLOG_VERSION
/**
 * @brief Adapter for spdlog logging framework
 * 
 * Integrates CQL logging with spdlog, one of the most popular C++ logging libraries.
 * 
 * Usage:
 * @code
 * #include <spdlog/spdlog.h>
 * #include <cql/logger_adapters.hpp>
 * 
 * auto spdlog_logger = spdlog::stdout_color_mt("cql");
 * auto adapter = std::make_unique<cql::adapters::SpdlogAdapter>(spdlog_logger);
 * cql::LoggerManager::initialize(std::move(adapter));
 * @endcode
 */
class SpdlogAdapter : public LoggerInterface {
public:
    /**
     * @brief Constructor with spdlog logger
     * @param logger Shared pointer to spdlog logger
     */
    explicit SpdlogAdapter(std::shared_ptr<spdlog::logger> logger);
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;

private:
    std::shared_ptr<spdlog::logger> m_logger;
    
    static spdlog::level::level_enum convert_level(LogLevel level);
};
#endif // SPDLOG_VERSION

#ifdef GLOG_VERSION
/**
 * @brief Adapter for Google glog logging framework
 * 
 * Integrates CQL logging with Google's glog library.
 * 
 * Usage:
 * @code
 * #include <glog/logging.h>
 * #include <cql/logger_adapters.hpp>
 * 
 * google::InitGoogleLogging("myapp");
 * auto adapter = std::make_unique<cql::adapters::GlogAdapter>();
 * cql::LoggerManager::initialize(std::move(adapter));
 * @endcode
 */
class GlogAdapter : public LoggerInterface {
public:
    GlogAdapter() = default;
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;

private:
    static int convert_level(LogLevel level);
};
#endif // GLOG_VERSION

#ifdef BOOST_LOG_VERSION
/**
 * @brief Adapter for Boost.Log logging framework
 * 
 * Integrates CQL logging with Boost.Log library.
 * 
 * Usage:
 * @code
 * #include <boost/log/trivial.hpp>
 * #include <cql/logger_adapters.hpp>
 * 
 * auto adapter = std::make_unique<cql::adapters::BoostLogAdapter>();
 * cql::LoggerManager::initialize(std::move(adapter));
 * @endcode
 */
class BoostLogAdapter : public LoggerInterface {
public:
    BoostLogAdapter() = default;
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;

private:
    static boost::log::trivial::severity_level convert_level(LogLevel level);
};
#endif // BOOST_LOG_VERSION

/**
 * @brief File-based logger implementation
 * 
 * A simple file logger that writes messages to a specified file.
 * Useful when you want file logging without external dependencies.
 * 
 * Usage:
 * @code
 * auto file_logger = std::make_unique<cql::adapters::FileLogger>("app.log");
 * file_logger->set_min_level(cql::LogLevel::INFO);
 * cql::LoggerManager::initialize(std::move(file_logger));
 * @endcode
 */
class FileLogger : public LoggerInterface {
public:
    /**
     * @brief Constructor with file path
     * @param file_path Path to the log file
     * @param append If true, append to existing file; if false, truncate
     */
    explicit FileLogger(const std::string& file_path, bool append = true);
    
    /**
     * @brief Destructor that ensures file is closed
     */
    ~FileLogger() override;
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;
    
    /**
     * @brief Set the minimum log level
     * @param min_level Messages below this level will be filtered out
     */
    void set_min_level(LogLevel min_level);
    
    /**
     * @brief Enable or disable automatic flushing after each message
     * @param enable true to enable auto-flush, false to buffer messages
     */
    void set_auto_flush(bool enable);

private:
    std::ofstream m_file;
    std::mutex m_file_mutex;
    LogLevel m_min_level{LogLevel::DEBUG};
    bool m_auto_flush{true};
    
    std::string format_message(LogLevel level, const std::string& message) const;
    static std::string get_timestamp();
};

/**
 * @brief Multi-output logger that forwards messages to multiple loggers
 * 
 * Allows sending log messages to multiple destinations simultaneously
 * (e.g., console + file, or multiple files).
 * 
 * Usage:
 * @code
 * auto multi_logger = std::make_unique<cql::adapters::MultiLogger>();
 * multi_logger->add_logger(std::make_unique<cql::DefaultConsoleLogger>());
 * multi_logger->add_logger(std::make_unique<cql::adapters::FileLogger>("app.log"));
 * cql::LoggerManager::initialize(std::move(multi_logger));
 * @endcode
 */
class MultiLogger : public LoggerInterface {
public:
    MultiLogger() = default;
    
    /**
     * @brief Add a logger to the multi-logger
     * @param logger Logger to add (takes ownership)
     */
    void add_logger(std::unique_ptr<LoggerInterface> logger);
    
    /**
     * @brief Remove all loggers
     */
    void clear_loggers();
    
    /**
     * @brief Get the number of configured loggers
     * @return Number of loggers
     */
    size_t logger_count() const;
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;

private:
    std::vector<std::unique_ptr<LoggerInterface>> m_loggers;
    mutable std::mutex m_loggers_mutex;
};

/**
 * @brief Async logger wrapper that processes log messages in a background thread
 * 
 * Wraps any logger implementation to make it asynchronous, improving performance
 * in high-throughput scenarios by avoiding blocking on I/O operations.
 * 
 * Usage:
 * @code
 * auto file_logger = std::make_unique<cql::adapters::FileLogger>("app.log");
 * auto async_logger = std::make_unique<cql::adapters::AsyncLogger>(std::move(file_logger));
 * cql::LoggerManager::initialize(std::move(async_logger));
 * @endcode
 */
class AsyncLogger : public LoggerInterface {
public:
    /**
     * @brief Constructor with underlying logger
     * @param logger Logger to wrap with async behavior (takes ownership)
     * @param queue_size Maximum number of messages to queue (default: 1000)
     */
    explicit AsyncLogger(std::unique_ptr<LoggerInterface> logger, size_t queue_size = 1000);
    
    /**
     * @brief Destructor that ensures all messages are processed
     */
    ~AsyncLogger() override;
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;

private:
    struct LogMessage {
        LogLevel level;
        std::string message;
    };
    
    std::unique_ptr<LoggerInterface> m_logger;
    std::queue<LogMessage> m_message_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;
    std::thread m_worker_thread;
    std::atomic<bool> m_shutdown{false};
    size_t m_max_queue_size;
    
    void worker_thread_func();
};

} // namespace adapters
} // namespace cql
