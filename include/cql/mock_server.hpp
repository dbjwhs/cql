// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_MOCK_SERVER_HPP
#define CQL_MOCK_SERVER_HPP

#include <string>
#include <functional>
#include <thread>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>

namespace cql::test {

/**
 * @class MockServer
 * @brief A simple HTTP mock server for testing API client implementations,
 * 
 * This mock server can be used to test API clients without making actual
 * network requests. It captures requests and returns predefined responses.
 */
class MockServer {
public:
    /**
     * @brief Constructor that starts the server on the specified port
     * @param port Port number to listen on
     */
    explicit MockServer(int port = 8080);
    
    /**
     * @brief Destructor that stops the server
     */
    ~MockServer();
    
    /**
     * @brief Add a response handler for a specific endpoint
     * @param endpoint The endpoint path (e.g., "/v1/messages")
     * @param handler Function that takes a request body and returns a response
     */
    void add_handler(const std::string& endpoint, 
                    std::function<std::string(const std::string&)> handler);
    
    /**
     * @brief Set a default response for all unhandled endpoints
     * @param response The default response to return
     */
    void set_default_response(const std::string& response);
    
    /**
     * @brief Start the mock server
     */
    void start();
    
    /**
     * @brief Stop the mock server
     */
    void stop();
    
    /**
     * @brief Get the base URL of the mock server
     * @return The base URL as a string
     */
    std::string get_url() const;
    
    /**
     * @brief Get a list of received requests
     * @return Vector of request bodies
     */
    std::vector<std::string> get_requests() const;
    
    /**
     * @brief Check if the server is running
     * @return true if running, false otherwise
     */
    bool is_running() const;
    
private:
    int m_port;
    std::atomic<bool> m_running;
    std::thread m_server_thread;
    std::map<std::string, std::function<std::string(const std::string&)>> m_handlers;
    std::string m_default_response;
    std::vector<std::string> m_requests;
    mutable std::mutex m_mutex;
    
    void run_server() const;
};

/**
 * @brief Create a mock response for the Claude API
 * @param content The content to include in the response
 * @return JSON response string mimicking the Claude API
 */
std::string create_mock_claude_response(const std::string& content);

/**
 * @brief Create a mock error response
 * @param status_code HTTP status code
 * @param error_type Error type
 * @param error_message Error message
 * @return JSON error response string
 */
std::string create_mock_error_response(int status_code, 
                                      const std::string& error_type,
                                      const std::string& error_message);

} // namespace cql::test

#endif // CQL_MOCK_SERVER_HPP