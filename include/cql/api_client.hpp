// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_API_CLIENT_HPP
#define CQL_API_CLIENT_HPP

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include <string_view>
#include <future>
#include <optional>
#include "secure_string.hpp"

namespace cql {

// Forward declaration
struct ApiResponse;

/**
 * @brief Callback type for streaming responses
 * 
 * This callback is invoked when a chunk of data is received from a streaming API response.
 * @param chunk The response chunk received
 * @param is_first_chunk Whether this is the first chunk received
 * @param is_last_chunk Whether this is the last chunk received
 * @return bool Whether to continue streaming (true) or stop (false)
 */
using StreamingCallback = std::function<bool(const ApiResponse& chunk, bool is_first_chunk, bool is_last_chunk)>;

/**
 * @enum ApiClientStatus
 * @brief Status of the API client
 */
enum class ApiClientStatus {
    Ready,      ///< Client is ready to send requests
    Connecting, ///< Client is connecting to the API
    Processing, ///< Client is processing a request
    Error,      ///< Client has encountered an error
    RateLimited ///< Client is rate-limited
};

/**
 * @enum ApiErrorCategory
 * @brief Categories of API client errors for better error handling
 */
enum class ApiErrorCategory {
    None,           ///< No error
    Network,        ///< Network connectivity issues
    Authentication, ///< API key issues
    RateLimit,      ///< Rate limiting or quota
    Server,         ///< Server-side errors
    Timeout,        ///< Request timeout
    Client,         ///< Client-side errors
    Unknown         ///< Unknown errors
};

/**
 * @struct GeneratedFile
 * @brief Structure to represent a file generated from an API response
 */
struct GeneratedFile {
    std::string m_filename;  ///< Name of the generated file
    std::string m_language;  ///< Programming language of the file
    std::string m_content;   ///< Content of the file
    bool m_is_test;          ///< Whether the file is a test file
};

/**
 * @struct ApiResponse
 * @brief Structure to represent an API response
 */
struct ApiResponse {
    bool m_success;                           ///< Whether the request was successful
    int m_status_code;                        ///< HTTP status code
    std::string m_raw_response;               ///< Raw response body
    std::vector<GeneratedFile> m_generated_files; ///< Files extracted from the response
    std::string m_error_message;              ///< Error message, if any
    ApiErrorCategory m_error_category = ApiErrorCategory::None; ///< Category of error, if any
    bool m_is_streaming = false;              ///< Whether this is a streaming response
    bool m_is_complete = true;                ///< Whether the response is complete
    
    /**
     * @brief Check if the response contains an error
     * @return true if an error occurred, false otherwise
     */
    [[nodiscard]] bool has_error() const {
        return !m_success;
    }
    
    /**
     * @brief Get the main content from the raw response
     * @return The main content string
     */
    [[nodiscard]] std::string get_main_content() const {
        return m_raw_response;
    }
    
    /**
     * @brief Check if the error is retryable
     * @return true if the error is retryable, false otherwise
     */
    [[nodiscard]] bool is_retryable() const {
        return m_error_category == ApiErrorCategory::Network ||
               m_error_category == ApiErrorCategory::RateLimit ||
               m_error_category == ApiErrorCategory::Server;
    }
    
    /**
     * @brief Check if this is a streaming response
     * @return true if this is a streaming response, false otherwise
     */
    [[nodiscard]] bool is_streaming() const {
        return m_is_streaming;
    }
    
    /**
     * @brief Check if the response is complete
     * @return true if the response is complete, false otherwise
     */
    [[nodiscard]] bool is_complete() const {
        return m_is_complete;
    }
};

/**
 * @class Config
 * @brief Configuration for the API client
 */
class Config {
public:
    /**
     * @brief Default constructor
     */
    Config() = default;
    
    /**
     * @brief Copy constructor (explicitly handle SecureString)
     */
    Config(const Config& other) 
        : m_api_key(SecureString(other.m_api_key.data())),
          m_model(other.m_model),
          m_api_base_url(other.m_api_base_url),
          m_timeout(other.m_timeout),
          m_max_retries(other.m_max_retries),
          m_output_directory(other.m_output_directory),
          m_overwrite_existing(other.m_overwrite_existing),
          m_create_missing_dirs(other.m_create_missing_dirs),
          m_no_save(other.m_no_save),
          m_streaming_enabled(other.m_streaming_enabled),
          m_max_tokens(other.m_max_tokens),
          m_temperature(other.m_temperature) {}
    
    /**
     * @brief Move constructor
     */
    Config(Config&& other) noexcept = default;
    
    /**
     * @brief Copy assignment (explicitly handle SecureString)
     */
    Config& operator=(const Config& other) {
        if (this != &other) {
            m_api_key = SecureString(other.m_api_key.data());
            m_model = other.m_model;
            m_api_base_url = other.m_api_base_url;
            m_timeout = other.m_timeout;
            m_max_retries = other.m_max_retries;
            m_output_directory = other.m_output_directory;
            m_overwrite_existing = other.m_overwrite_existing;
            m_create_missing_dirs = other.m_create_missing_dirs;
            m_no_save = other.m_no_save;
            m_streaming_enabled = other.m_streaming_enabled;
            m_max_tokens = other.m_max_tokens;
            m_temperature = other.m_temperature;
        }
        return *this;
    }
    
    /**
     * @brief Move assignment
     */
    Config& operator=(Config&& other) noexcept = default;
    
    /**
     * @brief Load configuration from default locations (env vars, config file)
     * @return Config object with loaded settings
     */
    static Config load_from_default_locations();
    
    /**
     * @brief Load configuration from a specific file
     * @param filename Path to the configuration file
     * @return Config object with loaded settings
     */
    static Config load_from_file(const std::string& filename);
    
    /**
     * @brief Get the API key
     * @return API key string
     */
    [[nodiscard]] std::string get_api_key() const { return m_api_key.data(); }
    
    /**
     * @brief Get the API key securely (masked for logging)
     * @return Masked API key safe for logging
     */
    [[nodiscard]] std::string get_api_key_masked() const { return m_api_key.masked(); }
    
    /**
     * @brief Validate the API key format
     * @return true if the API key appears valid, false otherwise
     */
    [[nodiscard]] bool validate_api_key() const { 
        // Basic validation: Check if the key is not empty and has an expected format
        return !m_api_key.empty() && m_api_key.size() >= 30;
    }
    
    /**
     * @brief Get the model to use it for requests
     * @return Model name string
     */
    [[nodiscard]] std::string get_model() const { return m_model; }
    
    /**
     * @brief Get the timeout for requests in seconds
     * @return Timeout value
     */
    [[nodiscard]] int get_timeout() const { return m_timeout; }
    
    /**
     * @brief Get the maximum number of retries for failed requests
     * @return Maximum retries value
     */
    [[nodiscard]] int get_max_retries() const { return m_max_retries; }
    
    /**
     * @brief Get the output directory for generated files
     * @return Output directory path
     */
    [[nodiscard]] std::string get_output_directory() const { return m_output_directory; }
    
    /**
     * @brief Check if existing files should be overwritten
     * @return true if files should be overwritten, false otherwise
     */
    [[nodiscard]] bool should_overwrite_existing_files() const { return m_overwrite_existing; }
    
    /**
     * @brief Check if missing directories should be created
     * @return true if directories should be created, false otherwise
     */
    [[nodiscard]] bool should_create_missing_directories() const { return m_create_missing_dirs; }
    
    /**
     * @brief Check if files should be saved to disk
     * @return true if files should not be saved, false otherwise
     */
    [[nodiscard]] bool no_save_mode() const { return m_no_save; }
    
    /**
     * @brief Get the API base URL
     * @return Base URL string
     */
    [[nodiscard]] std::string get_api_base_url() const { return m_api_base_url; }
    
    /**
     * @brief Set the API key
     * @param api_key API key string
     */
    void set_api_key(const std::string& api_key) { m_api_key = SecureString(api_key); }
    
    /**
     * @brief Set the model to use for requests
     * @param model Model name string
     */
    void set_model(const std::string& model) { m_model = model; }
    
    /**
     * @brief Set the base URL for API requests
     * @param base_url Base URL string
     */
    void set_api_base_url(const std::string& base_url) { m_api_base_url = base_url; }
    
    /**
     * @brief Set the timeout for requests in seconds
     * @param timeout Timeout value
     */
    void set_timeout(int timeout) { m_timeout = timeout; }
    
    /**
     * @brief Set the maximum number of retries for failed requests
     * @param max_retries Maximum retries value
     */
    void set_max_retries(int max_retries) { m_max_retries = max_retries; }
    
    /**
     * @brief Set the output directory for generated files
     * @param directory Output directory path
     */
    void set_output_directory(const std::string& directory) { m_output_directory = directory; }
    
    /**
     * @brief Set whether existing files should be overwritten
     * @param overwrite true if files should be overwritten, false otherwise
     */
    void set_overwrite_existing_files(bool overwrite) { m_overwrite_existing = overwrite; }
    
    /**
     * @brief Set whether missing directories should be created
     * @param create true if directories should be created, false otherwise
     */
    void set_create_missing_directories(bool create) { m_create_missing_dirs = create; }
    
    /**
     * @brief Set whether files should be saved to disk
     * @param no_save true if files should not be saved, false otherwise
     */
    void set_no_save_mode(bool no_save) { m_no_save = no_save; }
    
    /**
     * @brief Set whether to enable streaming mode for requests
     * @param enable true to enable streaming, false otherwise
     */
    void set_streaming_enabled(bool enable) { m_streaming_enabled = enable; }
    
    /**
     * @brief Check if streaming mode is enabled
     * @return true if streaming is enabled, false otherwise
     */
    [[nodiscard]] bool is_streaming_enabled() const { return m_streaming_enabled; }
    
    /**
     * @brief Set the maximum tokens to request from the API
     * @param max_tokens Maximum number of tokens
     */
    void set_max_tokens(int max_tokens) { m_max_tokens = max_tokens; }
    
    /**
     * @brief Get the maximum tokens to request from the API
     * @return Maximum tokens value
     */
    [[nodiscard]] int get_max_tokens() const { return m_max_tokens; }
    
    /**
     * @brief Set the temperature for API requests
     * @param temperature Temperature value between 0 and 1
     */
    void set_temperature(float temperature) { m_temperature = temperature; }
    
    /**
     * @brief Get the temperature for API requests
     * @return Temperature value
     */
    [[nodiscard]] float get_temperature() const { return m_temperature; }

private:
    SecureString m_api_key;          ///< API key for authentication (secured)
    std::string m_model = "claude-3-opus"; ///< Model to use for requests
    std::string m_api_base_url = "https://api.anthropic.com"; ///< Base URL for API requests
    int m_timeout = 60;              ///< Timeout in seconds
    int m_max_retries = 3;           ///< Maximum number of retries
    std::string m_output_directory;  ///< Directory to save generated files
    bool m_overwrite_existing = false; ///< Whether to overwrite existing files
    bool m_create_missing_dirs = true; ///< Whether to create missing directories
    bool m_no_save = false;          ///< Whether to save files to disk
    bool m_streaming_enabled = false; ///< Whether to use streaming mode for requests
    int m_max_tokens = 100000;       ///< Maximum number of tokens to request
    float m_temperature = 0.7f;      ///< Temperature for API requests
};

/**
 * @class ApiClient
 * @brief Client for communicating with the Claude API
 */
class ApiClient {
public:
    /**
     * @brief Constructor
     * @param config Configuration for the API client
     */
    explicit ApiClient(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~ApiClient();
    
    /**
     * @brief Submit a query to the Claude API
     * @param query The compiled query to submit
     * @return ApiResponse containing the result
     */
    [[nodiscard]] ApiResponse submit_query(const std::string& query) const;
    
    /**
     * @brief Submit a query to the Claude API asynchronously
     * @param query The compiled query to submit
     * @param callback Function to call with the complete response
     * @return std::future<ApiResponse> Future for the final response
     */
    [[nodiscard]] std::future<ApiResponse> submit_query_async(
        const std::string& query, 
        const std::function<void(ApiResponse)>& callback = nullptr) const;
    
    /**
     * @brief Submit a query to the Claude API with streaming responses
     * @param query The compiled query to submit
     * @param callback Function to call for each chunk of the response
     * @return ApiResponse containing the initial status
     */
    [[nodiscard]] ApiResponse submit_query_streaming(
        const std::string& query, 
        const StreamingCallback& callback) const;
    
    /**
     * @brief Submit a query to the Claude API asynchronously with streaming
     * @param query The compiled query to submit
     * @param callback Function to call for each chunk of the response
     * @return std::future<ApiResponse> Future for the final aggregated response
     */
    [[nodiscard]] std::future<ApiResponse> submit_query_streaming_async(
        const std::string& query, 
        const StreamingCallback& callback) const;
    
    /**
     * @brief Set the model to use for requests
     * @param model Model name string
     */
    void set_model(const std::string& model);
    
    /**
     * @brief Set the API key
     * @param api_key API key string
     */
    void set_api_key(const std::string& api_key);

    /**
     * @brief Set the timeout for requests in seconds
     * @param timeout_seconds Timeout value
     */
    void set_timeout(int timeout_seconds);
    
    /**
     * @brief Set the maximum number of retries for failed requests
     * @param max_retries Maximum retries value
     */
    void set_max_retries(int max_retries);
    
    /**
     * @brief Set the temperature for API requests
     * @param temperature Temperature value between 0 and 1
     */
    void set_temperature(float temperature);
    
    /**
     * @brief Set the maximum tokens to request from the API
     * @param max_tokens Maximum number of tokens
     */
    void set_max_tokens(int max_tokens);
    
    /**
     * @brief Enable or disable streaming mode
     * @param enable true to enable streaming, false to disable
     */
    void set_streaming_enabled(bool enable);
    
    /**
     * @brief Check if the client is connected to the API
     * @return true if connected, false otherwise
     */
    [[nodiscard]] bool is_connected() const;
    
    /**
     * @brief Get the current status of the API client
     * @return ApiClientStatus enum value
     */
    [[nodiscard]] ApiClientStatus get_status() const;
    
    /**
     * @brief Get the last error message
     * @return Error message string
     */
    [[nodiscard]] std::string get_last_error() const;
    
private:
    /// Private implementation details
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace cql

#endif // CQL_API_CLIENT_HPP