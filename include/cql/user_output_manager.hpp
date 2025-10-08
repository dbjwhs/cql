// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "user_output.hpp"
#include <memory>
#include <mutex>
#include <atomic>
#include <sstream>

namespace cql {

/**
 * @brief Central manager for user-facing output
 *
 * Provides a singleton interface for managing user output throughout
 * the CQL library. This separates user-visible messages from debug
 * logging, allowing clean separation of concerns.
 *
 * Usage:
 * @code
 * // Initialize with default console output
 * UserOutputManager::initialize();
 *
 * // Use throughout the application
 * UserOutputManager::success("File processed successfully");
 * UserOutputManager::error("Failed to open file");
 * UserOutputManager::info("Processing: ", filename);
 *
 * // Or use custom output
 * auto custom_output = std::make_unique<FileUserOutput>("output.txt");
 * UserOutputManager::initialize(std::move(custom_output));
 * @endcode
 */
class UserOutputManager {
    friend class TemporaryUserOutput;
public:
    /**
     * @brief Initialize with default console output
     */
    static void initialize();

    /**
     * @brief Initialize with custom output implementation
     * @param output Custom output instance (takes ownership)
     */
    static void initialize(std::unique_ptr<UserOutputInterface> output);

    /**
     * @brief Initialize with callback
     * @param callback Function to call for each output message
     */
    static void initialize_with_callback(UserOutputCallback callback);

    /**
     * @brief Initialize with null output (suppress all user messages)
     */
    static void initialize_null();

    /**
     * @brief Check if the output manager has been initialized
     */
    static bool is_initialized();

    /**
     * @brief Get the current output instance
     * @return Reference to the current output
     * @throws std::runtime_error if not initialized
     */
    static UserOutputInterface& get_output();

    /**
     * @brief Shutdown the output system
     */
    static void shutdown();

    // ========================================================================
    // Convenience methods for common message types
    // ========================================================================

    /**
     * @brief Output an informational message
     */
    template<typename... Args>
    static void info(const Args&... args) {
        write_formatted(MessageType::INFO, args...);
    }

    /**
     * @brief Output a success message
     */
    template<typename... Args>
    static void success(const Args&... args) {
        write_formatted(MessageType::SUCCESS, args...);
    }

    /**
     * @brief Output a warning message
     */
    template<typename... Args>
    static void warning(const Args&... args) {
        write_formatted(MessageType::WARNING, args...);
    }

    /**
     * @brief Output an error message
     */
    template<typename... Args>
    static void error(const Args&... args) {
        write_formatted(MessageType::ERROR, args...);
    }

    /**
     * @brief Output a progress message
     */
    template<typename... Args>
    static void progress(const Args&... args) {
        write_formatted(MessageType::PROGRESS, args...);
    }

    /**
     * @brief Write a message of any type
     */
    static void write(MessageType type, const std::string& message);

    /**
     * @brief Flush the current output
     */
    static void flush();

    /**
     * @brief Check if a message type is enabled
     */
    static bool is_enabled(MessageType type);

    /**
     * @brief Display an interactive prompt and wait for user input
     *
     * This helper function centralizes interactive prompt handling,
     * ensuring proper synchronization between std::cout and std::cin.
     * Interactive prompts intentionally use std::cout (not UserOutputManager)
     * to maintain synchronization with std::cin and ensure the prompt appears
     * before the input cursor.
     *
     * @param prompt The prompt message to display
     * @return The user's input as a string (trimmed of leading/trailing whitespace)
     */
    static std::string prompt(const std::string& prompt_message);

    /**
     * @brief Output a formatted list of items
     *
     * Displays a list of items with consistent formatting. Each item is displayed
     * on a separate line with the specified prefix.
     *
     * @tparam Container Type of the container (vector, set, etc.)
     * @param type Message type for the list output
     * @param items Container of items to display
     * @param prefix Prefix for each item (default: "  - ")
     * @param header Optional header to display before the list
     */
    template<typename Container>
    static void list(MessageType type, const Container& items,
                     const std::string& prefix = "  - ",
                     const std::string& header = "") {
        if (!header.empty()) {
            write(type, header);
        }
        for (const auto& item : items) {
            std::ostringstream oss;
            oss << prefix << item;
            write(type, oss.str());
        }
    }

private:
    static std::unique_ptr<UserOutputInterface> s_output;
    static std::mutex s_output_mutex;
    static std::atomic<bool> s_initialized;
    static std::unique_ptr<ConsoleUserOutput> s_fallback_output;

    // Initialize fallback output for use when main output is not available
    static void ensure_fallback_output();

    // Helper method for formatted output
    template<typename... Args>
    static void write_formatted(MessageType type, const Args&... args) {
        if (!is_enabled(type)) {
            return;
        }

        std::ostringstream oss;
        (oss << ... << args);
        write(type, oss.str());
    }

    // Prevent instantiation
    UserOutputManager() = delete;
    ~UserOutputManager() = delete;
    UserOutputManager(const UserOutputManager&) = delete;
    UserOutputManager& operator=(const UserOutputManager&) = delete;
};

/**
 * @brief RAII helper for temporary output configuration
 *
 * Allows temporarily switching to a different output for a specific scope,
 * then automatically restoring the previous output when destroyed.
 */
class TemporaryUserOutput {
public:
    /**
     * @brief Constructor that switches to a temporary output
     * @param temp_output Temporary output to use
     */
    explicit TemporaryUserOutput(std::unique_ptr<UserOutputInterface> temp_output);

    /**
     * @brief Destructor that restores the previous output
     */
    ~TemporaryUserOutput();

    // Non-copyable and non-movable
    TemporaryUserOutput(const TemporaryUserOutput&) = delete;
    TemporaryUserOutput& operator=(const TemporaryUserOutput&) = delete;
    TemporaryUserOutput(TemporaryUserOutput&&) = delete;
    TemporaryUserOutput& operator=(TemporaryUserOutput&&) = delete;

private:
    std::unique_ptr<UserOutputInterface> m_previous_output;
    bool m_had_previous_output;
};

} // namespace cql

// Convenience macros for user output (optional, for users who prefer macros)
#define CQL_USER_INFO(...) ::cql::UserOutputManager::info(__VA_ARGS__)
#define CQL_USER_SUCCESS(...) ::cql::UserOutputManager::success(__VA_ARGS__)
#define CQL_USER_WARNING(...) ::cql::UserOutputManager::warning(__VA_ARGS__)
#define CQL_USER_ERROR(...) ::cql::UserOutputManager::error(__VA_ARGS__)
#define CQL_USER_PROGRESS(...) ::cql::UserOutputManager::progress(__VA_ARGS__)
