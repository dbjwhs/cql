// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/user_output.hpp"
#include <cstdio>
#include <cstring>

namespace cql {

// ============================================================================
// ConsoleUserOutput Implementation
// ============================================================================

ConsoleUserOutput::ConsoleUserOutput() {
    // Auto-detect color support (simplified - check if stdout is a TTY)
    m_colored_output = isatty(fileno(stdout));
}

void ConsoleUserOutput::write(MessageType type, const std::string& message) {
    if (!is_enabled(type)) {
        return;
    }

    std::ostream& stream = get_stream(type);

    if (m_colored_output) {
        stream << get_color_code(type) << get_prefix(type) << message << "\033[0m" << std::endl;
    } else {
        stream << get_prefix(type) << message << std::endl;
    }
}

void ConsoleUserOutput::flush() {
    std::cout.flush();
    std::cerr.flush();
}

bool ConsoleUserOutput::is_enabled(MessageType type) const {
    return m_type_enabled[static_cast<int>(type)];
}

void ConsoleUserOutput::set_colored_output(bool enable) {
    m_colored_output = enable;
}

void ConsoleUserOutput::set_type_enabled(MessageType type, bool enable) {
    m_type_enabled[static_cast<int>(type)] = enable;
}

std::string ConsoleUserOutput::get_color_code(MessageType type) const {
    switch (type) {
        case MessageType::INFO:     return "\033[36m";    // Cyan
        case MessageType::SUCCESS:  return "\033[32m";    // Green
        case MessageType::WARNING:  return "\033[33m";    // Yellow
        case MessageType::ERROR:    return "\033[31m";    // Red
        case MessageType::PROGRESS: return "\033[34m";    // Blue
        default:                    return "";
    }
}

std::string ConsoleUserOutput::get_prefix(MessageType type) const {
    switch (type) {
        case MessageType::SUCCESS:  return "✓ ";
        case MessageType::WARNING:  return "⚠ ";
        case MessageType::ERROR:    return "✗ ";
        default:                    return "";
    }
}

std::ostream& ConsoleUserOutput::get_stream(MessageType type) const {
    // Errors and warnings go to stderr, everything else to stdout
    if (type == MessageType::ERROR || type == MessageType::WARNING) {
        return std::cerr;
    }
    return std::cout;
}

// ============================================================================
// FileUserOutput Implementation
// ============================================================================

FileUserOutput::FileUserOutput(const std::string& file_path, bool append)
    : m_file_path(file_path) {
    const char* mode = append ? "a" : "w";
    m_file = fopen(file_path.c_str(), mode);
}

FileUserOutput::~FileUserOutput() {
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
}

void FileUserOutput::write(MessageType type, const std::string& message) {
    if (!is_open() || !is_enabled(type)) {
        return;
    }

    const std::string type_str = message_type_to_string(type);
    fprintf(m_file, "[%s] %s\n", type_str.c_str(), message.c_str());
}

void FileUserOutput::flush() {
    if (m_file) {
        fflush(m_file);
    }
}

bool FileUserOutput::is_enabled(MessageType type) const {
    return m_type_enabled[static_cast<int>(type)];
}

bool FileUserOutput::is_open() const {
    return m_file != nullptr;
}

// ============================================================================
// MultiUserOutput Implementation
// ============================================================================

void MultiUserOutput::write(MessageType type, const std::string& message) {
    for (auto& output : m_outputs) {
        if (output && output->is_enabled(type)) {
            output->write(type, message);
        }
    }
}

void MultiUserOutput::flush() {
    for (auto& output : m_outputs) {
        if (output) {
            output->flush();
        }
    }
}

bool MultiUserOutput::is_enabled(MessageType type) const {
    // Enabled if ANY output has this type enabled
    for (const auto& output : m_outputs) {
        if (output && output->is_enabled(type)) {
            return true;
        }
    }
    return false;
}

void MultiUserOutput::add_output(std::unique_ptr<UserOutputInterface> output) {
    if (output) {
        m_outputs.push_back(std::move(output));
    }
}

// ============================================================================
// CallbackUserOutput Implementation
// ============================================================================

CallbackUserOutput::CallbackUserOutput(UserOutputCallback callback)
    : m_callback(std::move(callback)) {}

void CallbackUserOutput::write(MessageType type, const std::string& message) {
    if (m_callback) {
        m_callback(type, message.c_str());
    }
}

void CallbackUserOutput::flush() {
    // No-op for callback-based output
}

bool CallbackUserOutput::is_enabled(MessageType /*type*/) const {
    return m_callback != nullptr;
}

} // namespace cql
