// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_PROJECT_UTILS_HPP
#define CQL_PROJECT_UTILS_HPP

// standard library includes
#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <fstream>
#include <random>
#include <cstddef>
#include <sstream>
#include <filesystem>
#include <iomanip>

// version information
#define PROJECT_VERSION_MAJOR 1
#define PROJECT_VERSION_MINOR 0

// utility macros (use sparingly)
#define DECLARE_NON_COPYABLE(ClassType) \
ClassType(const ClassType&) = delete; \
ClassType& operator=(const ClassType&) = delete

#define DECLARE_NON_MOVEABLE(ClassType) \
ClassType(ClassType&) = delete; \
ClassType& operator=(ClassType&) = delete

// common constants
constexpr std::size_t DEFAULT_BUFFER_SIZE = 1024;
constexpr double EPSILON = 1e-6;

// transform thread id to std::string, note if no argument
// current thread id, else passed in thread id
template<typename ThreadType = std::thread::id>
inline std::string threadIdToString(ThreadType thread_id = std::this_thread::get_id()) {
    std::stringstream ss;
    ss << thread_id;
    return ss.str();
}

// simple random generator for int's
class RandomGenerator {
private:
    std::mt19937 m_gen;
    std::uniform_int_distribution<int> m_dist;  // for integers
    // or
    // std::uniform_real_distribution<double> m_dist; // for floating point

public:
    RandomGenerator(const int min, const int max)
        : m_gen(std::random_device{}())
        , m_dist(min, max) {}

    int getNumber() {
        return m_dist(m_gen);
    }

    // delete copy and move operations
    DECLARE_NON_COPYABLE(RandomGenerator);
    DECLARE_NON_MOVEABLE(RandomGenerator);
};

enum class LogLevel {
    INFO,
    NORMAL,
    DEBUG,
    ERROR,
    CRITICAL
};

class Logger {
private:
    // shared_ptr to maintain the singleton
    inline static std::shared_ptr<Logger> m_instance;
    inline static std::mutex m_instance_mutex; // mutex for thread-safe initialization

    // helper method to handle the common logging logic
    void write_log_message(const LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // write to a file if file logging is enabled for this level
        if (is_level_enabled(level) && m_file_output_enabled) {
            m_log_file << message;
            m_log_file.flush();
        }

        // write to the console if console logging is enabled for this level
        if (is_level_enabled(level)) {
            if ((level == LogLevel::CRITICAL || level == LogLevel::ERROR) && m_stderr_enabled) {
                std::cerr << message;
            } else { // info, normal, debug
                std::cout << message;
            }
        }
    }

    // helper to build the log prefix
    static std::stringstream create_log_prefix(LogLevel level) {
        std::stringstream message;
        message << get_utc_timestamp()
                << " [" << log_level_to_string(level)
                << "] [Thread:"
                << std::this_thread::get_id() << "] ";
        return message;
    }

    // constructor is now private to control instantiation
    explicit Logger(const std::string& path) {
        if (!std::filesystem::exists(std::filesystem::path(path).parent_path())) {
            throw std::runtime_error("Invalid path provided: " + path);
        }

        m_log_file.open(path, std::ios::app);
        if (!m_log_file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + path);
        }

        // initialize enabled levels - all levels enabled by default
        for (auto & m_enabled_level : m_enabled_levels) {
            m_enabled_level = true;
        }
    }

public:
    // raii class for temporarily disabling stderr output
    class StderrSuppressionGuard {
    public:
        StderrSuppressionGuard() : m_was_enabled(Logger::getInstance().isStderrEnabled()) {
            Logger::getInstance().disableStderr();
        }

        ~StderrSuppressionGuard() {
            if (m_was_enabled) {
                Logger::getInstance().enableStderr();
            }
        }

    private:
        bool m_was_enabled;
    };

    // private method to get or create the instance
    static std::shared_ptr<Logger> getOrCreateInstance(const std::string& path = "../custom.log") {
        std::lock_guard<std::mutex> lock(m_instance_mutex);
        if (!m_instance) {
            m_instance = std::shared_ptr<Logger>(new Logger(path));
        }
        return m_instance;
    }

    // returns a reference for backward compatibility but uses shared_ptr internally
    static Logger& getInstance() {
        return *getOrCreateInstance();
    }

    // custom path version of getinstance
    static Logger& getInstance(const std::string& custom_path) {
        return *getOrCreateInstance(custom_path);
    }

    // new method for code that explicitly wants to manage the shared_ptr
    static std::shared_ptr<Logger> getInstancePtr() {
        return getOrCreateInstance();
    }

    // with a custom path for the shared_ptr version
    static std::shared_ptr<Logger> getInstancePtr(const std::string& custom_path) {
        return getOrCreateInstance(custom_path);
    }

    // destructor remains the same
    ~Logger() {
        if (m_log_file.is_open()) {
            m_log_file.close();
        }
    }

    // the rest of the methods remain unchanged
    // variadic template for logging
    template<typename... Args>
    void log(const LogLevel level, const Args&... args) {
        if (!is_level_enabled(level)) {
            return;
        }
        auto message = create_log_prefix(level);
        (message << ... << args);
        message << std::endl;
        write_log_message(level, message.str());
    }

    // overload for logging with depth
    template<typename... Args>
    void log_with_depth(const LogLevel level, const int depth, const Args&... args) {
        if (!is_level_enabled(level)) {
            return;
        }
        auto message = create_log_prefix(level);
        message << getIndentation(depth);
        (message << ... << args);
        message << std::endl;
        write_log_message(level, message.str());
    }

    // enable/disable specific log level
    void setLevelEnabled(LogLevel level, bool enabled) {
        int levelIndex = static_cast<int>(level);
        if (levelIndex >= 0 && levelIndex <= static_cast<int>(LogLevel::CRITICAL)) {
            m_enabled_levels[levelIndex] = enabled;
        }
    }

    // check if a specific log level is enabled
    bool isLevelEnabled(LogLevel level) const {
        return is_level_enabled(level);
    }

    // disable stderr output
    void disableStderr() {
        m_stderr_enabled = false;
    }

    // enable stderr output
    void enableStderr() {
        m_stderr_enabled = true;
    }

    // get current stderr output state
    bool isStderrEnabled() const {
        return m_stderr_enabled;
    }

    // enable/disable file output
    void setFileOutputEnabled(bool enabled) {
        m_file_output_enabled = enabled;
    }

    // check if file output is enabled
    bool isFileOutputEnabled() const {
        return m_file_output_enabled;
    }

private:
    std::ofstream m_log_file;
    std::mutex m_mutex;
    std::atomic<bool> m_stderr_enabled{true};
    std::atomic<bool> m_file_output_enabled{true};
    std::atomic<bool> m_enabled_levels[5]{true, true, true, true, true}; // one for each log level

    // check if a level is enabled (internal helper)
    bool is_level_enabled(LogLevel level) const {
        int levelIndex = static_cast<int>(level);
        if (levelIndex >= 0 && levelIndex <= static_cast<int>(LogLevel::CRITICAL)) {
            return m_enabled_levels[levelIndex];
        }
        return false;
    }

    // utility function for expression tree visualization
    static std::string getIndentation(const int depth) {
        return {static_cast<char>(depth * 2), ' '};
    }

    // convert log level to string
    static std::string log_level_to_string(const LogLevel level) {
        switch (level) {
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::NORMAL:
                return "NORMAL";
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::ERROR:
                return "ERROR";
            case LogLevel::CRITICAL:
                return "CRITICAL";
            default:
                return "UNKNOWN";
        }
    }

    // get current utc timestamp
    static std::string get_utc_timestamp() {
        const auto now = std::chrono::system_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        auto time = std::chrono::system_clock::to_time_t(now);

        struct tm tm_buf{};
#ifdef _WIN32
        // ### not tested (did work a few years ago)
        gmtime_s(&tm_buf, &time);
#else
        gmtime_r(&time, &tm_buf);
#endif

        std::stringstream ss;
        ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << " UTC";
        return ss.str();
    }
};

/**
 * Platform-specific clipboard operations
 */
namespace clipboard {
    /**
     * Copy text to the system clipboard
     * 
     * @param text Text to copy to clipboard
     * @return True if the operation was successful
     */
    bool copy_to_clipboard(const std::string& text);
    
    /**
     * Get text from the system clipboard
     * 
     * @return Clipboard content as string
     */
    std::string get_from_clipboard();
}

#endif // CQL_PROJECT_UTILS_HPP