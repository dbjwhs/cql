// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/user_output_manager.hpp"
#include <stdexcept>

namespace cql {

// Static member initialization
std::unique_ptr<UserOutputInterface> UserOutputManager::s_output = nullptr;
std::mutex UserOutputManager::s_output_mutex;
std::atomic<bool> UserOutputManager::s_initialized{false};
std::unique_ptr<ConsoleUserOutput> UserOutputManager::s_fallback_output = nullptr;

void UserOutputManager::initialize() {
    std::lock_guard<std::mutex> lock(s_output_mutex);
    s_output = std::make_unique<ConsoleUserOutput>();
    s_initialized = true;
}

void UserOutputManager::initialize(std::unique_ptr<UserOutputInterface> output) {
    std::lock_guard<std::mutex> lock(s_output_mutex);
    s_output = std::move(output);
    s_initialized = true;
}

void UserOutputManager::initialize_with_callback(UserOutputCallback callback) {
    std::lock_guard<std::mutex> lock(s_output_mutex);
    s_output = std::make_unique<CallbackUserOutput>(callback);
    s_initialized = true;
}

void UserOutputManager::initialize_null() {
    std::lock_guard<std::mutex> lock(s_output_mutex);
    s_output = std::make_unique<NullUserOutput>();
    s_initialized = true;
}

bool UserOutputManager::is_initialized() {
    return s_initialized.load();
}

UserOutputInterface& UserOutputManager::get_output() {
    if (!is_initialized()) {
        ensure_fallback_output();
        return *s_fallback_output;
    }

    std::lock_guard<std::mutex> lock(s_output_mutex);
    if (!s_output) {
        throw std::runtime_error("UserOutputManager output is null despite being initialized");
    }
    return *s_output;
}

void UserOutputManager::shutdown() {
    std::lock_guard<std::mutex> lock(s_output_mutex);
    if (s_output) {
        s_output->flush();
        s_output.reset();
    }
    s_initialized = false;
}

void UserOutputManager::write(MessageType type, const std::string& message) {
    if (!is_initialized()) {
        ensure_fallback_output();
        s_fallback_output->write(type, message);
        return;
    }

    std::lock_guard<std::mutex> lock(s_output_mutex);
    if (s_output) {
        s_output->write(type, message);
    } else {
        // Fallback in case output was cleared
        ensure_fallback_output();
        s_fallback_output->write(type, message);
    }
}

void UserOutputManager::flush() {
    if (!is_initialized()) {
        ensure_fallback_output();
        s_fallback_output->flush();
        return;
    }

    std::lock_guard<std::mutex> lock(s_output_mutex);
    if (s_output) {
        s_output->flush();
    }
}

bool UserOutputManager::is_enabled(MessageType type) {
    if (!is_initialized()) {
        return true; // Fallback output has all types enabled
    }

    std::lock_guard<std::mutex> lock(s_output_mutex);
    if (s_output) {
        return s_output->is_enabled(type);
    }
    return true; // Default to enabled if output is null
}

std::string UserOutputManager::prompt(const std::string& prompt_message) {
    // Interactive prompts use std::cout directly for proper std::cin synchronization
    std::cout << prompt_message;
    std::cout.flush();

    std::string input;
    std::getline(std::cin, input);

    // Trim leading and trailing whitespace
    size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }

    size_t end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
}

void UserOutputManager::ensure_fallback_output() {
    static std::mutex fallback_mutex;
    std::lock_guard<std::mutex> lock(fallback_mutex);

    if (!s_fallback_output) {
        s_fallback_output = std::make_unique<ConsoleUserOutput>();
    }
}

// ============================================================================
// TemporaryUserOutput Implementation
// ============================================================================

TemporaryUserOutput::TemporaryUserOutput(std::unique_ptr<UserOutputInterface> temp_output)
    : m_had_previous_output(UserOutputManager::is_initialized()) {

    if (m_had_previous_output) {
        std::lock_guard<std::mutex> lock(UserOutputManager::s_output_mutex);
        m_previous_output = std::move(UserOutputManager::s_output);
        UserOutputManager::s_output = std::move(temp_output);
    } else {
        UserOutputManager::initialize(std::move(temp_output));
    }
}

TemporaryUserOutput::~TemporaryUserOutput() {
    if (m_had_previous_output) {
        std::lock_guard<std::mutex> lock(UserOutputManager::s_output_mutex);
        UserOutputManager::s_output = std::move(m_previous_output);
    } else {
        UserOutputManager::shutdown();
    }
}

} // namespace cql
