// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <iostream>

namespace cql {

/**
 * @brief Message types for user-facing output
 *
 * Categorizes messages by their purpose, allowing different presentation
 * styles or routing for different message types.
 */
enum class MessageType {
    INFO,      // General informational messages
    SUCCESS,   // Success confirmations
    WARNING,   // Warning messages (non-fatal issues)
    ERROR,     // Error messages (user-facing, not debug)
    PROGRESS   // Progress indicators and status updates
};

/**
 * @brief Convert MessageType to string representation
 */
inline std::string message_type_to_string(MessageType type) {
    switch (type) {
        case MessageType::INFO: return "INFO";
        case MessageType::SUCCESS: return "SUCCESS";
        case MessageType::WARNING: return "WARNING";
        case MessageType::ERROR: return "ERROR";
        case MessageType::PROGRESS: return "PROGRESS";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Abstract interface for user-facing output
 *
 * This interface separates user-facing messages from debug logging.
 * User output is meant to be seen by end users during normal operation,
 * while debug logging (LoggerInterface) is for developers and diagnostics.
 *
 * Key distinction:
 * - UserOutput: "Processing complete", "File saved to output.txt"
 * - Logger: "Parsed 1234 tokens in 45ms", "Cache hit for template X"
 */
class UserOutputInterface {
public:
    virtual ~UserOutputInterface() = default;

    /**
     * @brief Output a user-facing message
     * @param type The type/category of the message
     * @param message The message to display to the user
     */
    virtual void write(MessageType type, const std::string& message) = 0;

    /**
     * @brief Flush any buffered output
     */
    virtual void flush() = 0;

    /**
     * @brief Check if output is enabled for a specific message type
     * @param type The message type to check
     * @return true if enabled, false otherwise
     */
    virtual bool is_enabled(MessageType type) const = 0;
};

/**
 * @brief Console-based user output implementation
 *
 * Outputs user messages to stdout (for info/success/progress) and
 * stderr (for warnings/errors). Provides optional colored output
 * for better visual distinction.
 */
class ConsoleUserOutput : public UserOutputInterface {
public:
    ConsoleUserOutput();
    ~ConsoleUserOutput() override = default;

    void write(MessageType type, const std::string& message) override;
    void flush() override;
    bool is_enabled(MessageType type) const override;

    /**
     * @brief Enable or disable colored output
     * @param enable true to enable colors, false for plain text
     */
    void set_colored_output(bool enable);

    /**
     * @brief Enable or disable specific message types
     * @param type Message type to configure
     * @param enable true to enable, false to disable
     */
    void set_type_enabled(MessageType type, bool enable);

private:
    bool m_colored_output{true};
    bool m_type_enabled[5]{true, true, true, true, true}; // One per MessageType

    std::string get_color_code(MessageType type) const;
    std::string get_prefix(MessageType type) const;
    std::ostream& get_stream(MessageType type) const;
};

/**
 * @brief File-based user output implementation
 *
 * Writes user messages to a file. Useful for capturing user-visible
 * output in scripts or batch processing scenarios.
 */
class FileUserOutput : public UserOutputInterface {
public:
    /**
     * @brief Constructor
     * @param file_path Path to the output file
     * @param append If true, append to existing file; if false, truncate
     */
    explicit FileUserOutput(const std::string& file_path, bool append = false);
    ~FileUserOutput() override;

    void write(MessageType type, const std::string& message) override;
    void flush() override;
    bool is_enabled(MessageType type) const override;

    /**
     * @brief Check if the file is open and ready for writing
     */
    bool is_open() const;

private:
    std::string m_file_path;
    FILE* m_file{nullptr};
    bool m_type_enabled[5]{true, true, true, true, true};
};

/**
 * @brief Null output implementation that discards all messages
 *
 * Useful for quiet mode or testing scenarios where user output
 * should be suppressed.
 */
class NullUserOutput : public UserOutputInterface {
public:
    void write(MessageType /*type*/, const std::string& /*message*/) override {}
    void flush() override {}
    bool is_enabled(MessageType /*type*/) const override { return false; }
};

/**
 * @brief Multi-output implementation that writes to multiple outputs
 *
 * Allows routing user messages to multiple destinations simultaneously
 * (e.g., both console and file).
 */
class MultiUserOutput : public UserOutputInterface {
public:
    MultiUserOutput() = default;
    ~MultiUserOutput() override = default;

    void write(MessageType type, const std::string& message) override;
    void flush() override;
    bool is_enabled(MessageType type) const override;

    /**
     * @brief Add an output destination
     * @param output Output implementation to add
     */
    void add_output(std::unique_ptr<UserOutputInterface> output);

    /**
     * @brief Get the number of registered outputs
     */
    size_t output_count() const { return m_outputs.size(); }

private:
    std::vector<std::unique_ptr<UserOutputInterface>> m_outputs;
};

/**
 * @brief Callback-based user output implementation
 *
 * Allows integration with custom output systems via callbacks.
 */
using UserOutputCallback = std::function<void(MessageType, const char*)>;

class CallbackUserOutput : public UserOutputInterface {
public:
    explicit CallbackUserOutput(UserOutputCallback callback);

    void write(MessageType type, const std::string& message) override;
    void flush() override;
    bool is_enabled(MessageType type) const override;

private:
    UserOutputCallback m_callback;
};

} // namespace cql
