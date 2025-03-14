// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "../../include/cql/api_client.hpp"
#include "../../include/cql/project_utils.hpp"

namespace cql {

// Private implementation structure for ApiClient
struct ApiClient::Impl {
    Config m_config;                         ///< Client configuration
    ApiClientStatus m_status;                ///< Current client status
    std::string m_last_error;                ///< Last error message
    CURL* m_curl;                            ///< CURL handle
    std::string m_response_buffer;           ///< Buffer for response data
    bool m_initialized;                      ///< Whether the client is initialized
    int m_current_retry = 0;                 ///< Current retry attempt
    double m_retry_delay = 1.0;              ///< Initial retry delay in seconds
    
    // Streaming-related members
    mutable std::mutex m_mutex;                             ///< Mutex for thread safety in streaming
    mutable std::condition_variable m_cv;                   ///< Condition variable for signaling
    mutable std::atomic<bool> m_streaming_active{false};    ///< Whether streaming is active
    mutable StreamingCallback m_streaming_callback;         ///< Callback for streaming responses

    explicit Impl(const Config& config) 
        : m_config(config),
          m_status(ApiClientStatus::Ready),
          m_curl(nullptr),
          m_initialized(false) {
        
        // Initialize CURL
        curl_global_init(CURL_GLOBAL_ALL);
        m_curl = curl_easy_init();
        
        if (m_curl) {
            m_initialized = true;
            Logger::getInstance().log(LogLevel::INFO, "ApiClient initialized with model: ", config.get_model());
        } else {
            m_status = ApiClientStatus::Error;
            m_last_error = "Failed to initialize CURL";
            Logger::getInstance().log(LogLevel::ERROR, "Failed to initialize CURL");
        }
    }

    ~Impl() {
        // Clean up CURL
        if (m_curl) {
            curl_easy_cleanup(m_curl);
        }
        curl_global_cleanup();
    }

    // Callback function for CURL to write response data
    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
        size_t real_size = size * nmemb;
        buffer->append(static_cast<char*>(contents), real_size);
        return real_size;
    }
    
    // Callback function for CURL to write streaming response data
    static size_t streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        auto* self = static_cast<ApiClient::Impl*>(userp);
        size_t real_size = size * nmemb;
        
        // Lock to safely update shared data
        std::lock_guard<std::mutex> lock(self->m_mutex);
        
        // Append the data to the buffer
        std::string chunk(static_cast<char*>(contents), real_size);
        
        // Check if we have complete JSON events (separated by newlines)
        std::istringstream stream(chunk);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty() || line == "\r") continue;
            
            // Process the line (SSE format: "data: {...}")
            if (line.substr(0, 6) == "data: ") {
                std::string json_str = line.substr(6);
                
                // Handle the end of stream marker
                if (json_str == "[DONE]") {
                    if (self->m_streaming_callback) {
                        // Create final empty response
                        ApiResponse final_response;
                        final_response.m_success = true;
                        final_response.m_is_streaming = true;
                        final_response.m_is_complete = true;
                        
                        // Call callback with final chunk (marked as last)
                        self->m_streaming_callback(final_response, false, true);
                    }
                    continue;
                }
                
                try {
                    nlohmann::json event = nlohmann::json::parse(json_str);
                    
                    // Create a response object for this chunk
                    ApiResponse chunk_response;
                    chunk_response.m_success = true;
                    chunk_response.m_is_streaming = true;
                    chunk_response.m_is_complete = false;
                    
                    // Extract content from the event
                    if (event.contains("type") && event["type"] == "content_block_delta" && 
                        event.contains("delta") && event["delta"].contains("text")) {
                        chunk_response.m_raw_response = event["delta"]["text"];
                    }
                    
                    // Call the streaming callback if it exists, with info about first/last chunk
                    static bool is_first_chunk = true;
                    if (self->m_streaming_callback) {
                        bool continue_streaming = self->m_streaming_callback(
                            chunk_response, is_first_chunk, false);
                            
                        // Respect callback's decision to continue or stop
                        if (!continue_streaming) {
                            return 0; // Returning 0 will cause CURL to abort the transfer
                        }
                    }
                    
                    is_first_chunk = false;
                    
                } catch (const std::exception& e) {
                    Logger::getInstance().log(LogLevel::ERROR, 
                        "Error parsing streaming JSON: ", e.what());
                }
            }
        }
        
        return real_size;
    }

    // Prepare the request for the Claude API
    struct curl_slist* prepare_request(const std::string& query, std::string& request_data, bool streaming = false) {
        // Construct the request JSON
        nlohmann::json request_json = {
            {"model", m_config.get_model()},
            {"max_tokens", m_config.get_max_tokens()},
            {"temperature", m_config.get_temperature()},
            {"messages", {
                {
                    {"role", "user"},
                    {"content", query}
                }
            }}
        };
        
        // Add streaming option if requested
        if (streaming || m_config.is_streaming_enabled()) {
            request_json["stream"] = true;
        }
        
        request_data = request_json.dump();
        
        // Construct the full URL by combining base URL and endpoint
        std::string api_url = m_config.get_api_base_url() + "/v1/messages";
        
        // Set up CURL request
        curl_easy_setopt(m_curl, CURLOPT_URL, api_url.c_str());
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_response_buffer);
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request_data.c_str());
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_config.get_timeout());
        
        // Set headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        const std::string auth_header = "x-api-key: " + m_config.get_api_key();
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        
        return headers;
    }

    // Execute the API request
    CURLcode execute_request(struct curl_slist* headers) const {
        // Perform the request
        Logger::getInstance().log(LogLevel::INFO, "Sending request to Claude API...");
        const CURLcode res = curl_easy_perform(m_curl);
        
        // Cleanup headers
        curl_slist_free_all(headers);
        
        return res;
    }

    // Process the API response
    ApiResponse process_response(CURLcode curl_result, const ApiResponse& partial_response) {
        ApiResponse response = partial_response;
        
        // Check for CURL errors
        if (curl_result != CURLE_OK) {
            m_status = ApiClientStatus::Error;
            m_last_error = curl_easy_strerror(curl_result);
            response.m_error_message = m_last_error;
            response.m_status_code = 0;
            
            // Categorize CURL errors
            switch(curl_result) {
                case CURLE_OPERATION_TIMEDOUT:
                    response.m_error_category = ApiErrorCategory::Timeout;
                    break;
                case CURLE_COULDNT_CONNECT:
                case CURLE_COULDNT_RESOLVE_HOST:
                case CURLE_COULDNT_RESOLVE_PROXY:
                case CURLE_INTERFACE_FAILED:
                    response.m_error_category = ApiErrorCategory::Network;
                    break;
                case CURLE_SSL_CONNECT_ERROR:
                case CURLE_PEER_FAILED_VERIFICATION:
                case CURLE_SSL_CERTPROBLEM:
                    response.m_error_category = ApiErrorCategory::Network;
                    break;
                default:
                    response.m_error_category = ApiErrorCategory::Unknown;
                    break;
            }
            
            Logger::getInstance().log(LogLevel::ERROR, "CURL error: ", m_last_error, 
                " (Category: ", static_cast<int>(response.m_error_category), ")");
            return response;
        }
        
        // Get status code
        long status_code;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &status_code);
        response.m_status_code = static_cast<int>(status_code);
        
        // Process response based on status code
        if (status_code >= 200 && status_code < 300) {
            return process_successful_response(response);
        } else if (status_code == 429) {
            return process_rate_limited_response(response);
        } else if (status_code == 401 || status_code == 403) {
            response.m_error_category = ApiErrorCategory::Authentication;
            return process_error_response(response, status_code);
        } else if (status_code >= 500) {
            response.m_error_category = ApiErrorCategory::Server;
            return process_error_response(response, status_code);
        } else {
            response.m_error_category = ApiErrorCategory::Client;
            return process_error_response(response, status_code);
        }
    }
    
    // Process a successful API response (status code 2xx)
    ApiResponse process_successful_response(ApiResponse& response) {
        m_status = ApiClientStatus::Ready;
        response.m_success = true;
        
        try {
            // Parse JSON response
            nlohmann::json json_response = nlohmann::json::parse(m_response_buffer);
            
            // Extract content from response
            if (json_response.contains("content") && json_response["content"].is_array()) {
                // Extract content from response
                for (const auto& content : json_response["content"]) {
                    if (content.contains("text") && content["text"].is_string()) {
                        response.m_raw_response = content["text"];
                        break;
                    }
                }
            } else {
                // If we can't find content in the expected format, extract just the response content
                // without including metadata like query or model information
                if (json_response.contains("completion") && json_response["completion"].is_string()) {
                    response.m_raw_response = json_response["completion"];
                } else {
                    // Default to empty string if we can't extract proper content
                    // This prevents metadata leakage
                    response.m_raw_response = "{}";
                    Logger::getInstance().log(LogLevel::ERROR, "Cannot extract content from API response");
                }
            }
            Logger::getInstance().log(LogLevel::INFO, "API request successful");
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error parsing API response: ", e.what());
            // Set to empty JSON object instead of raw buffer to prevent metadata leakage
            response.m_raw_response = "{}";
        }
        
        return response;
    }
    
    // Process a rate-limited API response (status code 429)
    ApiResponse process_rate_limited_response(ApiResponse& response) {
        m_status = ApiClientStatus::RateLimited;
        response.m_error_message = "Rate limited: Too many requests";
        response.m_error_category = ApiErrorCategory::RateLimit;
        Logger::getInstance().log(LogLevel::ERROR, "API rate limit exceeded (429)");
        return response;
    }
    
    // Process an error API response (status code >= 400, except 429)
    ApiResponse process_error_response(ApiResponse& response, long status_code) {
        m_status = ApiClientStatus::Error;
        
        try {
            // Try to parse error from JSON
            nlohmann::json json_response = nlohmann::json::parse(m_response_buffer);
            if (json_response.contains("error") && json_response["error"].is_object()) {
                auto error = json_response["error"];
                if (error.contains("message") && error["message"].is_string()) {
                    m_last_error = error["message"];
                }
            }
        } catch (...) {
            // If parsing fails, use the raw response as error
            m_last_error = "API error " + std::to_string(status_code) + ": " + m_response_buffer;
        }
        
        response.m_error_message = m_last_error;
        Logger::getInstance().log(LogLevel::ERROR, "API error (", status_code, "): ", m_last_error);
        return response;
    }

    // Send a request to the Claude API - main entry point
    ApiResponse send_request(const std::string& query) {
        // Reset retry counters
        m_current_retry = 0;
        m_retry_delay = 1.0;
        
        return send_request_with_retry(query);
    }
    
    // Send a streaming request to the Claude API
    ApiResponse send_streaming_request(const std::string& query, const StreamingCallback& callback) {
        // Reset retry counters
        m_current_retry = 0;
        m_retry_delay = 1.0;
        
        m_streaming_callback = callback;
        m_streaming_active = true;
        
        // Set up initial response
        ApiResponse initial_response;
        initial_response.m_success = true;
        initial_response.m_is_streaming = true;
        initial_response.m_is_complete = false;
        
        try {
            // Reset response buffer
            m_response_buffer.clear();
            
            // Set client status
            m_status = ApiClientStatus::Processing;
            
            // Prepare request with streaming enabled
            std::string request_data;
            struct curl_slist* headers = prepare_request(query, request_data, true);
            
            // Set up CURL for streaming
            curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, streaming_write_callback);
            curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
            
            // Execute request
            Logger::getInstance().log(LogLevel::INFO, "Sending streaming request to Claude API...");
            const CURLcode curl_result = curl_easy_perform(m_curl);
            
            // Clean up headers
            curl_slist_free_all(headers);
            
            // Update response based on a result
            if (curl_result != CURLE_OK) {
                m_status = ApiClientStatus::Error;
                m_last_error = curl_easy_strerror(curl_result);
                initial_response.m_success = false;
                initial_response.m_error_message = m_last_error;
                
                // Categorize CURL errors
                if (curl_result == CURLE_OPERATION_TIMEDOUT) {
                    initial_response.m_error_category = ApiErrorCategory::Timeout;
                } else if (curl_result == CURLE_COULDNT_CONNECT || 
                           curl_result == CURLE_COULDNT_RESOLVE_HOST ||
                           curl_result == CURLE_INTERFACE_FAILED) {
                    initial_response.m_error_category = ApiErrorCategory::Network;
                } else {
                    initial_response.m_error_category = ApiErrorCategory::Unknown;
                }
                
                Logger::getInstance().log(LogLevel::ERROR, "CURL error in streaming: ", m_last_error);
                
                // Final callback with error
                if (callback) {
                    callback(initial_response, true, true);
                }
            } else {
                // Get status code
                long status_code;
                curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &status_code);
                initial_response.m_status_code = static_cast<int>(status_code);
                
                if (status_code >= 200 && status_code < 300) {
                    m_status = ApiClientStatus::Ready;
                } else {
                    m_status = ApiClientStatus::Error;
                    initial_response.m_success = false;
                    initial_response.m_error_message = "HTTP error: " + std::to_string(status_code);
                    
                    // Categorize HTTP errors
                    if (status_code == 429) {
                        initial_response.m_error_category = ApiErrorCategory::RateLimit;
                    } else if (status_code == 401 || status_code == 403) {
                        initial_response.m_error_category = ApiErrorCategory::Authentication;
                    } else if (status_code >= 500) {
                        initial_response.m_error_category = ApiErrorCategory::Server;
                    } else {
                        initial_response.m_error_category = ApiErrorCategory::Client;
                    }
                    
                    // Final callback with error
                    if (callback) {
                        callback(initial_response, true, true);
                    }
                }
            }
            
        } catch (const std::exception& e) {
            m_status = ApiClientStatus::Error;
            m_last_error = e.what();
            initial_response.m_success = false;
            initial_response.m_error_message = e.what();
            initial_response.m_error_category = ApiErrorCategory::Client;
            
            Logger::getInstance().log(LogLevel::ERROR, "Exception in streaming request: ", e.what());
            
            // Final callback with error
            if (callback) {
                callback(initial_response, true, true);
            }
        }
        
        m_streaming_active = false;
        return initial_response;
    }
    
    // Send a request with retry logic
    ApiResponse send_request_with_retry(const std::string& query) {
        if (!m_initialized) {
            ApiResponse response;
            response.m_success = false;
            response.m_status_code = 0;
            response.m_error_message = "CURL not initialized";
            response.m_error_category = ApiErrorCategory::Client;
            return response;
        }

        // Reset response buffer
        m_response_buffer.clear();
        
        // Set client status
        m_status = ApiClientStatus::Processing;
        
        // Prepare request
        std::string request_data;
        struct curl_slist* headers = prepare_request(query, request_data);
        
        // Prepare response
        ApiResponse response;
        response.m_success = false;
        
        // Execute request
        CURLcode curl_result = execute_request(headers);
        
        // Process response
        response = process_response(curl_result, response);
        
        // Check if we should retry
        if (!response.m_success && response.is_retryable() && 
            m_current_retry < m_config.get_max_retries()) {
            
            // Log the retry with INFO level
            Logger::getInstance().log(LogLevel::INFO, 
                "Retrying request (", m_current_retry + 1, "/", 
                m_config.get_max_retries(), ") after ", m_retry_delay, " seconds");
            
            // Sleep with backoff
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(m_retry_delay * 1000)));
            
            // Increase retry counter and delay
            m_current_retry++;
            m_retry_delay *= 2.0; // Exponential backoff
            
            // Retry the request
            return send_request_with_retry(query);
        }
        
        return response;
    }
};

// ApiClient implementation

ApiClient::ApiClient(const Config& config) {
    if (!config.validate_api_key()) {
        // Log the error for debugging purposes
        Logger::getInstance().log(LogLevel::ERROR, 
            "API key is invalid or not set. ApiClient initialization failed.");
        throw std::runtime_error("Invalid API key configuration");
    }
    
    m_impl = std::make_unique<Impl>(config);
}

ApiClient::~ApiClient() = default;

ApiResponse ApiClient::submit_query(const std::string& query) const {
    try {
        Logger::getInstance().log(LogLevel::INFO, "Submitting query to Claude API");
        return m_impl->send_request(query);
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error submitting query: ", e.what());
        
        ApiResponse error_response{};
        error_response.m_success = false;
        error_response.m_error_message = e.what();
        return error_response;
    }
}

std::future<ApiResponse> ApiClient::submit_query_async(
    const std::string& query, 
    const std::function<void(ApiResponse)>& callback) const {
    
    Logger::getInstance().log(LogLevel::INFO, "Submitting query asynchronously to Claude API");
    
    // Launch the request in a separate thread using std::async
    return std::async(std::launch::async, [this, query, callback]() {
        ApiResponse response;
        
        try {
            // Execute the query
            response = m_impl->send_request(query);
            
            // Call the callback if provided
            if (callback) {
                callback(response);
            }
            
        } catch (const std::exception& e) {
            // Handle exceptions
            Logger::getInstance().log(LogLevel::ERROR, "Exception in async query: ", e.what());
            response.m_success = false;
            response.m_error_message = e.what();
            response.m_error_category = ApiErrorCategory::Client;
            
            if (callback) {
                callback(response);
            }
        }
        
        return response;
    });
}

ApiResponse ApiClient::submit_query_streaming(
    const std::string& query, 
    const StreamingCallback& callback) const {
    
    Logger::getInstance().log(LogLevel::INFO, "Submitting streaming query to Claude API");
    
    if (!callback) {
        ApiResponse error_response{};
        error_response.m_success = false;
        error_response.m_error_message = "Streaming callback cannot be null";
        error_response.m_error_category = ApiErrorCategory::Client;
        return error_response;
    }
    
    try {
        return m_impl->send_streaming_request(query, callback);
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Exception in streaming query: ", e.what());
        
        ApiResponse error_response{};
        error_response.m_success = false;
        error_response.m_error_message = e.what();
        error_response.m_error_category = ApiErrorCategory::Client;
        error_response.m_is_streaming = true;
        
        // Call the callback with the error
        callback(error_response, true, true);
        
        return error_response;
    }
}

std::future<ApiResponse> ApiClient::submit_query_streaming_async(
    const std::string& query, 
    const StreamingCallback& callback) const {
    
    Logger::getInstance().log(LogLevel::INFO, "Submitting streaming query asynchronously to Claude API");
    
    if (!callback) {
        std::promise<ApiResponse> promise;
        ApiResponse error_response;
        error_response.m_success = false;
        error_response.m_error_message = "Streaming callback cannot be null";
        error_response.m_error_category = ApiErrorCategory::Client;
        promise.set_value(error_response);
        return promise.get_future();
    }
    
    // Capture the aggregated response
    struct ResponseAggregator {
        std::string accumulated_text;
        bool had_error = false;
        std::string error_message;
        ApiErrorCategory error_category = ApiErrorCategory::None;
    };
    
    // Create a shared pointer to store the aggregated response across callbacks
    auto aggregator = std::make_shared<ResponseAggregator>();
    
    // Create a wrapper callback that both calls the user's callback and accumulates the response
    auto wrapper_callback = [callback, aggregator](const ApiResponse& chunk, bool is_first_chunk, bool is_last_chunk) {
        // Update the aggregator
        if (!chunk.m_success) {
            aggregator->had_error = true;
            aggregator->error_message = chunk.m_error_message;
            aggregator->error_category = chunk.m_error_category;
        } else {
            aggregator->accumulated_text += chunk.m_raw_response;
        }
        
        // Call the user's callback
        return callback(chunk, is_first_chunk, is_last_chunk);
    };
    
    // Launch the request in a separate thread using std::async
    return std::async(std::launch::async, [this, query, wrapper_callback, aggregator]() {
        ApiResponse final_response{};
        
        try {
            // Execute the streaming query
            m_impl->send_streaming_request(query, wrapper_callback);
            
            // Create the final response
            final_response.m_success = !aggregator->had_error;
            final_response.m_raw_response = aggregator->accumulated_text;
            
            if (aggregator->had_error) {
                final_response.m_error_message = aggregator->error_message;
                final_response.m_error_category = aggregator->error_category;
            }
            
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Exception in async streaming query: ", e.what());
            final_response.m_success = false;
            final_response.m_error_message = e.what();
            final_response.m_error_category = ApiErrorCategory::Client;
        }
        
        return final_response;
    });
}

void ApiClient::set_model(const std::string& model) const {
    m_impl->m_config.set_model(model);
}

void ApiClient::set_api_key(const std::string& api_key) const {
    m_impl->m_config.set_api_key(api_key);
}

void ApiClient::set_timeout(int timeout_seconds) const {
    m_impl->m_config.set_timeout(timeout_seconds);
}

void ApiClient::set_max_retries(int max_retries) const {
    m_impl->m_config.set_max_retries(max_retries);
}

void ApiClient::set_temperature(float temperature) const {
    m_impl->m_config.set_temperature(temperature);
}

void ApiClient::set_max_tokens(int max_tokens) const {
    m_impl->m_config.set_max_tokens(max_tokens);
}

void ApiClient::set_streaming_enabled(bool enable) const {
    m_impl->m_config.set_streaming_enabled(enable);
}

bool ApiClient::is_connected() const {
    return m_impl->m_initialized && 
           m_impl->m_status != ApiClientStatus::Error && 
           m_impl->m_status != ApiClientStatus::RateLimited;
}

ApiClientStatus ApiClient::get_status() const {
    return m_impl->m_status;
}

std::string ApiClient::get_last_error() const {
    return m_impl->m_last_error;
}

// Config implementation

Config Config::load_from_default_locations() {
    Config config{};
    
    // Try to load from environment variables first
    if (const char *api_key_env = std::getenv("CQL_API_KEY")) {
        config.m_api_key = api_key_env;
    }

    if (const char *model_env = std::getenv("CQL_MODEL")) {
        config.m_model = model_env;
    }
    
    if (const char *base_url_env = std::getenv("CQL_API_BASE_URL")) {
        config.m_api_base_url = base_url_env;
    }

    if (const char *timeout_env = std::getenv("CQL_TIMEOUT")) {
        try {
            config.m_timeout = std::stoi(timeout_env);
        } catch (...) {
            Logger::getInstance().log(LogLevel::ERROR, "Invalid timeout value in environment variable");
        }
    }

    if (const char *max_retries_env = std::getenv("CQL_MAX_RETRIES")) {
        try {
            config.m_max_retries = std::stoi(max_retries_env);
        } catch (...) {
            Logger::getInstance().log(LogLevel::ERROR, "Invalid max_retries value in environment variable");
        }
    }

    if (const char *output_dir_env = std::getenv("CQL_OUTPUT_DIR")) {
        config.m_output_directory = output_dir_env;
    }
    
    // Then try to load from a config file
    std::string home_dir;
    if (const char *home_env = std::getenv("HOME")) {
        home_dir = home_env;
    } else {
        home_dir = ".";  // Fallback to current directory
    }

    // Check if a config file exists
    if (const std::string config_path = home_dir + "/.cql/config.json"; std::filesystem::exists(config_path)) {
        try {
            config = load_from_file(config_path);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error loading config file: ", e.what());
        }
    }
    
    return config;
}

Config Config::load_from_file(const std::string& filename) {
    Config config;
    
    try {
        // Read the file
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + filename);
        }
        
        // Parse JSON
        nlohmann::json json_config;
        file >> json_config;
        
        // Extract values
        if (json_config.contains("api") && json_config["api"].is_object()) {
            auto api = json_config["api"];
            
            if (api.contains("key") && api["key"].is_string()) {
                config.m_api_key = api["key"];
            }
            
            if (api.contains("model") && api["model"].is_string()) {
                config.m_model = api["model"];
            }
            
            if (api.contains("base_url") && api["base_url"].is_string()) {
                config.m_api_base_url = api["base_url"];
            }
            
            if (api.contains("timeout") && api["timeout"].is_number()) {
                config.m_timeout = api["timeout"];
            }
            
            if (api.contains("max_retries") && api["max_retries"].is_number()) {
                config.m_max_retries = api["max_retries"];
            }
        }
        
        if (json_config.contains("output") && json_config["output"].is_object()) {
            auto output = json_config["output"];
            
            if (output.contains("default_directory") && output["default_directory"].is_string()) {
                config.m_output_directory = output["default_directory"];
                
                // Replace ~ with home directory
                if (!config.m_output_directory.empty() && config.m_output_directory[0] == '~') {
                    if (const char *home_env = std::getenv("HOME")) {
                        config.m_output_directory.replace(0, 1, home_env);
                    }
                }
            }
            
            if (output.contains("create_missing_dirs") && output["create_missing_dirs"].is_boolean()) {
                config.m_create_missing_dirs = output["create_missing_dirs"];
            }
            
            if (output.contains("overwrite_existing") && output["overwrite_existing"].is_boolean()) {
                config.m_overwrite_existing = output["overwrite_existing"];
            }
        }
        
        Logger::getInstance().log(LogLevel::INFO, "Loaded configuration from ", filename);
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error parsing config file: ", e.what());
        throw;
    }
    
    return config;
}

} // namespace cql