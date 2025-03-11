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

    // Prepare the request for the Claude API
    struct curl_slist* prepare_request(const std::string& query, std::string& request_data) {
        // Construct the request JSON
        nlohmann::json request_json = {
            {"model", m_config.get_model()},
            {"max_tokens", 100000},
            {"messages", {
                {
                    {"role", "user"},
                    {"content", query}
                }
            }}
        };
        
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
        std::string auth_header = "x-api-key: " + m_config.get_api_key();
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        
        return headers;
    }

    // Execute the API request
    CURLcode execute_request(struct curl_slist* headers) const {
        // Perform the request
        Logger::getInstance().log(LogLevel::INFO, "Sending request to Claude API...");
        CURLcode res = curl_easy_perform(m_curl);
        
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
        response.m_raw_response = m_response_buffer;
        
        try {
            // Parse JSON response
            nlohmann::json json_response = nlohmann::json::parse(m_response_buffer);
            if (json_response.contains("content") && json_response["content"].is_array()) {
                // Extract content from response
                for (const auto& content : json_response["content"]) {
                    if (content.contains("text") && content["text"].is_string()) {
                        response.m_raw_response = content["text"];
                        break;
                    }
                }
            }
            Logger::getInstance().log(LogLevel::INFO, "API request successful");
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error parsing API response: ", e.what());
            // Still mark as success since we got a valid HTTP response,
            // The response processor will handle extracting code blocks
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

ApiResponse ApiClient::submit_query_async(const std::string& query, 
                                        const std::function<void(ApiResponse)>& callback) const {
    // For now, implement the synchronous version
    // This is a placeholder for future async implementation
    Logger::getInstance().log(LogLevel::INFO, "Async API requests not yet implemented, falling back to sync");
    ApiResponse response = submit_query(query);
    
    // Call the callback with the response
    if (callback) {
        callback(response);
    }
    
    return response;
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
    Config config;
    
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
    
    std::string config_path = home_dir + "/.cql/config.json";
    
    // Check if a config file exists
    if (std::filesystem::exists(config_path)) {
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