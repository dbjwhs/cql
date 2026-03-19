// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_PROGRESS_DISPLAY_HPP
#define CQL_PROGRESS_DISPLAY_HPP

#include <string>
#include <thread>
#include <atomic>
#include <iostream>

namespace cql {

/**
 * @brief Simple spinner for non-streaming mode
 *
 * Displays a rotating spinner character on the console while
 * waiting for a response. Runs in a background thread.
 */
class ProgressDisplay {
public:
    explicit ProgressDisplay(std::string message = "Processing");
    ~ProgressDisplay();

    void start();
    void stop();

    // Not copyable or movable
    ProgressDisplay(const ProgressDisplay&) = delete;
    ProgressDisplay& operator=(const ProgressDisplay&) = delete;

private:
    void spin();

    std::string m_message;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
};

} // namespace cql

#endif // CQL_PROGRESS_DISPLAY_HPP
