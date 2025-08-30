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

#include "logger_bridge.hpp"

// For backward compatibility, expose historic LogLevel enum at global scope
using LogLevel = cql::HistoricLogLevel;

// Replace historic Logger with LoggerBridge (maintains exact same API)
using Logger = cql::LoggerBridge;

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
