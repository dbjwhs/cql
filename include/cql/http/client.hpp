// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <map>
#include <future>
#include <memory>
#include <chrono>
#include <optional>
#include <functional>
#include <vector>

namespace cql::http {

// HTTP Client Configuration Constants
namespace defaults {
    constexpr std::chrono::seconds DEFAULT_TIMEOUT{30};
    constexpr int MAX_REDIRECTS = 5;
    constexpr int MAX_CONNECTIONS = 10;
    constexpr size_t MAX_RESPONSE_SIZE = 100 * 1024 * 1024; // 100MB
    constexpr bool VERIFY_SSL = true;
    constexpr bool ENABLE_COOKIES = false;
    constexpr bool ENABLE_COMPRESSION = true;
}

/**
 * @brief HTTP request structure
 * 
 * Represents an HTTP request with all necessary components
 * for making API calls to AI providers.
 */
struct Request {
    std::string url;                                    ///< Full URL including protocol
    std::string method = "POST";                        ///< HTTP method (GET, POST, etc.)
    std::map<std::string, std::string> headers;         ///< HTTP headers
    std::string body;                                   ///< Request body (for POST/PUT)
    std::chrono::seconds timeout = defaults::DEFAULT_TIMEOUT;  ///< Request timeout
    int max_redirects = defaults::MAX_REDIRECTS;       ///< Maximum redirects to follow
    bool verify_ssl = defaults::VERIFY_SSL;            ///< Whether to verify SSL certificates
    std::optional<std::string> proxy;                  ///< Optional proxy URL
};

/**
 * @brief HTTP response structure
 * 
 * Contains the complete response from an HTTP request including
 * status code, headers, and body content.
 */
struct Response {
    int status_code = 0;                               ///< HTTP status code
    std::map<std::string, std::string> headers;        ///< Response headers
    std::string body;                                  ///< Response body
    std::chrono::milliseconds elapsed{0};              ///< Request elapsed time
    std::optional<std::string> error_message;          ///< Error message if request failed
    
    /**
     * @brief Check if response indicates success
     * @return true if status code is 2xx
     */
    [[nodiscard]] bool is_success() const {
        return status_code >= 200 && status_code < 300;
    }
    
    /**
     * @brief Check if response indicates client error
     * @return true if status code is 4xx
     */
    [[nodiscard]] bool is_client_error() const {
        return status_code >= 400 && status_code < 500;
    }
    
    /**
     * @brief Check if response indicates server error
     * @return true if status code is 5xx
     */
    [[nodiscard]] bool is_server_error() const {
        return status_code >= 500 && status_code < 600;
    }
};

/**
 * @brief Progress callback for streaming operations
 */
using ProgressCallback = std::function<void(size_t bytes_received, size_t total_bytes)>;

/**
 * @brief Streaming callback for Server-Sent Events (SSE)
 */
using StreamCallback = std::function<void(const std::string& chunk)>;

/**
 * @brief Abstract interface for HTTP client implementations
 * 
 * This interface defines the contract for HTTP client implementations.
 * It supports both synchronous and asynchronous operations, with
 * streaming support for Server-Sent Events (SSE) commonly used by AI APIs.
 * 
 * @note Implementations should handle connection pooling, retries,
 *       and proper resource cleanup internally.
 */
class ClientInterface {
public:
    virtual ~ClientInterface() = default;
    
    /**
     * @brief Send a synchronous HTTP request
     * 
     * @param req The request to send
     * @return Response containing the result
     * @throws std::runtime_error on network errors
     */
    [[nodiscard]] virtual Response send(const Request& req) = 0;
    
    /**
     * @brief Send an asynchronous HTTP request
     * 
     * @param req The request to send
     * @return Future that will contain the response
     * @note The future may throw exceptions on error
     */
    [[nodiscard]] virtual std::future<Response> send_async(const Request& req) = 0;
    
    /**
     * @brief Send a request with streaming response
     * 
     * Useful for Server-Sent Events (SSE) used by many AI APIs
     * for streaming responses.
     * 
     * @param req The request to send
     * @param callback Function called for each chunk received
     * @note Callback will be called from a background thread
     */
    virtual void send_stream(const Request& req, StreamCallback callback) = 0;
    
    /**
     * @brief Set a progress callback for large transfers
     * 
     * @param callback Function called periodically with progress updates
     */
    virtual void set_progress_callback(ProgressCallback callback) = 0;
    
    /**
     * @brief Check if the client is properly configured
     * 
     * @return true if client is ready for use
     */
    [[nodiscard]] virtual bool is_configured() const = 0;
    
    /**
     * @brief Get client implementation name
     * 
     * @return Implementation name (e.g., "CURL", "WinHTTP")
     */
    [[nodiscard]] virtual std::string get_implementation_name() const = 0;
};

/**
 * @brief Configuration for HTTP client
 */
struct ClientConfig {
    std::chrono::seconds default_timeout = defaults::DEFAULT_TIMEOUT;  ///< Default request timeout
    int max_connections = defaults::MAX_CONNECTIONS;   ///< Maximum concurrent connections
    int max_redirects = defaults::MAX_REDIRECTS;       ///< Maximum redirects to follow
    bool verify_ssl = defaults::VERIFY_SSL;            ///< SSL certificate verification
    std::optional<std::string> proxy;                  ///< Proxy server URL
    std::optional<std::string> ca_bundle_path;         ///< Custom CA bundle path
    std::map<std::string, std::string> default_headers; ///< Headers to include in all requests
    bool enable_cookies = defaults::ENABLE_COOKIES;    ///< Enable cookie jar
    bool enable_compression = defaults::ENABLE_COMPRESSION; ///< Enable gzip/deflate compression
    size_t max_response_size = defaults::MAX_RESPONSE_SIZE;  ///< Maximum response size
};

/**
 * @brief Factory for creating HTTP client instances
 * 
 * Creates platform-appropriate HTTP client implementations.
 */
class ClientFactory {
public:
    /**
     * @brief Create a default HTTP client
     * 
     * @param config Optional configuration
     * @return Unique pointer to client instance
     */
    [[nodiscard]] static std::unique_ptr<ClientInterface> create_default(
        const ClientConfig& config = ClientConfig{}
    );
    
    /**
     * @brief Create a CURL-based HTTP client
     * 
     * @param config Optional configuration
     * @return Unique pointer to CURL client instance
     * @throws std::runtime_error if CURL is not available
     */
    [[nodiscard]] static std::unique_ptr<ClientInterface> create_curl_client(
        const ClientConfig& config = ClientConfig{}
    );
    
    /**
     * @brief Get list of available client implementations
     * 
     * @return Vector of implementation names
     */
    [[nodiscard]] static std::vector<std::string> get_available_implementations();
};

} // namespace cql::http
