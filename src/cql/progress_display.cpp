// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/progress_display.hpp"
#include <chrono>

namespace cql {

ProgressDisplay::ProgressDisplay(std::string message)
    : m_message(std::move(message)) {}

ProgressDisplay::~ProgressDisplay() {
    stop();
}

void ProgressDisplay::start() {
    if (m_running.exchange(true)) {
        return; // Already running
    }
    m_thread = std::thread(&ProgressDisplay::spin, this);
}

void ProgressDisplay::stop() {
    if (!m_running.exchange(false)) {
        return; // Not running
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
    // Clear the spinner line
    std::cerr << "\r" << std::string(m_message.size() + 4, ' ') << "\r";
    std::cerr.flush();
}

void ProgressDisplay::spin() {
    const char spinner[] = {'|', '/', '-', '\\'};
    int frame = 0;

    while (m_running.load()) {
        std::cerr << "\r" << m_message << " " << spinner[frame % 4] << " ";
        std::cerr.flush();
        ++frame;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace cql
