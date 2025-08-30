// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <map>
#include <utility>
#include <vector>
#include <mutex>
#include <atomic>
#include <sstream>
#include <nlohmann/json.hpp>
#include "../../include/cql/project_utils.hpp"
#include "../../include/cql/mock_server.hpp"
#include "../../include/cql/json_utils.hpp"

namespace cql::test {

// Simple mock implementation that doesn't start a server
// In a real implementation, this would use a library like cpp-httplib or Boost.Beast
// to create an actual HTTP server

MockServer::MockServer(int port) 
    : m_port(port), 
      m_running(false), 
      m_default_response(R"({"error": "No handler for this endpoint"})") {
}

MockServer::~MockServer() {
    stop();
}

void MockServer::add_handler(const std::string& endpoint, std::function<std::string(const std::string&)> handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers[endpoint] = std::move(handler);
}

void MockServer::set_default_response(const std::string& response) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_default_response = response;
}

void MockServer::start() {
    if (!m_running) {
        m_running = true;
        m_server_thread = std::thread(&MockServer::run_server, this);
        Logger::getInstance().log(LogLevel::INFO, "Mock server started on port ", m_port);
    }
}

void MockServer::stop() {
    if (m_running) {
        m_running = false;
        if (m_server_thread.joinable()) {
            m_server_thread.join();
        }
        Logger::getInstance().log(LogLevel::INFO, "Mock server stopped");
    }
}

std::string MockServer::get_url() const {
    return "http://localhost:" + std::to_string(m_port);
}

std::vector<std::string> MockServer::get_requests() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_requests;
}

bool MockServer::is_running() const {
    return m_running;
}

// This is a simplified mock implementation that doesn't run a server
// It allows tests to directly call into the handler functions
void MockServer::run_server() const {
    Logger::getInstance().log(LogLevel::INFO, "Mock server is running (simulation)");
    
    // In a real implementation, this would set up a socket and listen for connections
    // For now; we just keep the thread alive as long as m_running is true
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Create a mock response for the Claude API
std::string create_mock_claude_response(const std::string& content) {
    nlohmann::json response = JsonUtils::create_mock_response(
        content,
        "claude-3-opus-20240229",
        "msg_mock123456789"
    );
    
    return JsonUtils::to_pretty_string(response);
}

// Create a mock error response
std::string create_mock_error_response(int status_code, 
                                     const std::string& error_type,
                                     const std::string& error_message) {
    nlohmann::json response = JsonUtils::create_error_response(
        status_code,
        error_type,
        error_message
    );
    
    return JsonUtils::to_pretty_string(response);
}

} // namespace cql::test
