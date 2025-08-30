// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../../include/cql/http/client.hpp"
#include "../../../include/cql/project_utils.hpp"
#include <curl/curl.h>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstring>
#include <random>
#include <cmath>

namespace cql::http {

// Implementation of RetryPolicy::calculate_delay
std::chrono::milliseconds RetryPolicy::calculate_delay(int attempt) const {
    // Calculate base delay with exponential backoff
    double delay_ms = initial_delay.count() * std::pow(backoff_multiplier, attempt);
    
    // Apply max delay cap
    delay_ms = std::min(delay_ms, static_cast<double>(max_delay.count()));
    
    // Add jitter if enabled to prevent thundering herd problem
    // Jitter adds random variation (±25%) to retry delays, preventing multiple
    // clients from retrying at exactly the same time after a service outage.
    // This helps distribute the load when many clients are retrying simultaneously.
    if (enable_jitter && delay_ms > 0) {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.75, 1.25);
        delay_ms *= dis(gen);
    }
    
    return std::chrono::milliseconds(static_cast<long>(delay_ms));
}

namespace {

// CURL write callback for response body
size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    std::string* response = static_cast<std::string*>(userdata);
    size_t total_size = size * nmemb;
    response->append(ptr, total_size);
    return total_size;
}

// CURL write callback for headers
size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
    size_t total_size = size * nitems;
    std::string header(buffer, total_size);
    
    // Remove trailing newline
    if (!header.empty() && header.back() == '\n') {
        header.pop_back();
    }
    if (!header.empty() && header.back() == '\r') {
        header.pop_back();
    }
    
    // Parse header (key: value)
    size_t colon_pos = header.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = header.substr(0, colon_pos);
        std::string value = header.substr(colon_pos + 1);
        
        // Trim whitespace from value
        size_t first = value.find_first_not_of(" \t");
        if (first != std::string::npos) {
            value = value.substr(first);
        }
        
        (*headers)[key] = value;
    }
    
    return total_size;
}

// CURL progress callback
int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                     curl_off_t /* ultotal */, curl_off_t /* ulnow */) {
    auto* callback = static_cast<ProgressCallback*>(clientp);
    if (callback && *callback) {
        (*callback)(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
    }
    return 0; // Return 0 to continue
}

// CURL streaming callback for SSE
size_t stream_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* callback = static_cast<StreamCallback*>(userdata);
    size_t total_size = size * nmemb;
    
    if (callback && *callback) {
        std::string chunk(ptr, total_size);
        (*callback)(chunk);
    }
    
    return total_size;
}

} // anonymous namespace

/**
 * @brief CURL-based HTTP client implementation
 */
class CurlClient : public ClientInterface {
public:
    explicit CurlClient(const ClientConfig& config);
    ~CurlClient() override;
    
    Response send(const Request& req) override;
    std::future<Response> send_async(const Request& req) override;
    void send_stream(const Request& req, StreamCallback callback) override;
    void set_progress_callback(ProgressCallback callback) override;
    bool is_configured() const override;
    std::string get_implementation_name() const override;
    
private:
    void configure_curl(CURL* curl, const Request& req, 
                       std::string& response_body,
                       std::map<std::string, std::string>& response_headers);
    Response execute_request(CURL* curl, std::string& response_body,
                            std::map<std::string, std::string>& response_headers);
    
    ClientConfig m_config;
    ProgressCallback m_progress_callback;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_initialized{false};
    
    // CURL multi handle for connection pooling
    CURLM* m_multi_handle = nullptr;
    
    // Initialize CURL library (once per process)
    static std::once_flag s_curl_init_flag;
    static void init_curl_library();
};

// Static initialization
std::once_flag CurlClient::s_curl_init_flag;

void CurlClient::init_curl_library() {
    curl_global_init(CURL_GLOBAL_ALL);
    Logger::getInstance().log(LogLevel::INFO, "CURL library initialized");
}

CurlClient::CurlClient(const ClientConfig& config) 
    : m_config(config) {
    
    // Initialize CURL library once
    std::call_once(s_curl_init_flag, &CurlClient::init_curl_library);
    
    // Create multi handle for connection pooling
    m_multi_handle = curl_multi_init();
    if (!m_multi_handle) {
        throw std::runtime_error("Failed to initialize CURL multi handle");
    }
    
    // Set connection pool size
    curl_multi_setopt(m_multi_handle, CURLMOPT_MAXCONNECTS, 
                      static_cast<long>(config.max_connections));
    
    m_initialized = true;
    Logger::getInstance().log(LogLevel::INFO, 
        "CurlClient initialized with timeout: ", config.default_timeout.count(), "s");
}

CurlClient::~CurlClient() {
    if (m_multi_handle) {
        curl_multi_cleanup(m_multi_handle);
    }
}

void CurlClient::configure_curl(CURL* curl, const Request& req,
                               std::string& response_body,
                               std::map<std::string, std::string>& response_headers) {
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, req.url.c_str());
    
    // Set HTTP method
    if (req.method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req.body.size());
    } else if (req.method == "GET") {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (req.method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
    } else if (req.method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (req.method == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
    }
    
    // Set headers
    struct curl_slist* headers = nullptr;
    
    // Add default headers from config
    for (const auto& [key, value] : m_config.default_headers) {
        std::string header = key + ": " + value;
        headers = curl_slist_append(headers, header.c_str());
    }
    
    // Add request-specific headers
    for (const auto& [key, value] : req.headers) {
        std::string header = key + ": " + value;
        headers = curl_slist_append(headers, header.c_str());
    }
    
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    // Set callbacks
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
    
    // Set timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 
                    static_cast<long>(req.timeout.count()));
    
    // Set redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 
                    static_cast<long>(req.max_redirects));
    
    // SSL verification
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, req.verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, req.verify_ssl ? 2L : 0L);
    
    // Custom CA bundle if specified
    if (m_config.ca_bundle_path) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, m_config.ca_bundle_path->c_str());
    }
    
    // Proxy
    if (req.proxy) {
        curl_easy_setopt(curl, CURLOPT_PROXY, req.proxy->c_str());
    } else if (m_config.proxy) {
        curl_easy_setopt(curl, CURLOPT_PROXY, m_config.proxy->c_str());
    }
    
    // Compression
    if (m_config.enable_compression) {
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
    }
    
    // Progress callback
    if (m_progress_callback) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &m_progress_callback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }
    
    // User agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CQL-HTTP-Client/1.0");
}

Response CurlClient::execute_request(CURL* curl, std::string& response_body,
                                    std::map<std::string, std::string>& response_headers) {
    Response response;
    auto start_time = std::chrono::steady_clock::now();
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    auto end_time = std::chrono::steady_clock::now();
    response.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    if (res != CURLE_OK) {
        response.error_message = curl_easy_strerror(res);
        Logger::getInstance().log(LogLevel::ERROR, 
            "CURL request failed: ", response.error_message.value());
        
        // Set appropriate status code based on error
        if (res == CURLE_COULDNT_CONNECT || res == CURLE_COULDNT_RESOLVE_HOST) {
            response.status_code = 0; // Network error
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
            response.status_code = 408; // Request Timeout
        } else {
            response.status_code = 500; // Internal Server Error
        }
    } else {
        // Get HTTP status code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        response.status_code = static_cast<int>(http_code);
        
        response.body = std::move(response_body);
        response.headers = std::move(response_headers);
        
        Logger::getInstance().log(LogLevel::INFO, 
            "HTTP request completed with status: ", response.status_code);
    }
    
    return response;
}

Response CurlClient::send(const Request& req) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        throw std::runtime_error("CurlClient not initialized");
    }
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Sending ", req.method, " request to: ", req.url);
    
    Response last_response;
    int attempt = 0;
    
    // Retry loop with exponential backoff
    while (attempt <= req.retry_policy.max_retries) {
        // Create CURL handle for each attempt (ensures clean state)
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to create CURL handle");
        }
        
        // Ensure cleanup
        struct CurlCleanup {
            CURL* handle;
            struct curl_slist* headers = nullptr;
            ~CurlCleanup() {
                if (headers) curl_slist_free_all(headers);
                if (handle) curl_easy_cleanup(handle);
            }
        } cleanup{curl};
        
        std::string response_body;
        std::map<std::string, std::string> response_headers;
        
        configure_curl(curl, req, response_body, response_headers);
        
        // Execute the request
        last_response = execute_request(curl, response_body, response_headers);
        
        // Check if we should retry
        if (!RetryPolicy::should_retry(last_response.status_code) || 
            attempt >= req.retry_policy.max_retries) {
            // Success or max retries reached
            if (attempt > 0) {
                Logger::getInstance().log(LogLevel::INFO,
                    "Request completed after ", attempt, " retries");
            }
            return last_response;
        }
        
        // Calculate delay before retry
        auto delay = req.retry_policy.calculate_delay(attempt);
        
        Logger::getInstance().log(LogLevel::INFO,
            "Request failed with status ", last_response.status_code,
            ", retrying in ", delay.count(), "ms (attempt ", attempt + 1,
            " of ", req.retry_policy.max_retries, ")");
        
        // Wait before retry
        std::this_thread::sleep_for(delay);
        
        attempt++;
    }
    
    // Should not reach here, but return last response just in case
    return last_response;
}

std::future<Response> CurlClient::send_async(const Request& req) {
    return std::async(std::launch::async, [this, req]() {
        return this->send(req);
    });
}

void CurlClient::send_stream(const Request& req, StreamCallback callback) {
    if (!m_initialized) {
        throw std::runtime_error("CurlClient not initialized");
    }
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Starting streaming request to: ", req.url);
    
    // Create CURL handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to create CURL handle");
    }
    
    // Ensure cleanup
    struct CurlCleanup {
        CURL* handle;
        struct curl_slist* headers = nullptr;
        ~CurlCleanup() {
            if (headers) curl_slist_free_all(headers);
            if (handle) curl_easy_cleanup(handle);
        }
    } cleanup{curl};
    
    std::string response_body; // Not used for streaming
    std::map<std::string, std::string> response_headers;
    
    configure_curl(curl, req, response_body, response_headers);
    
    // Override write callback for streaming
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stream_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Streaming request failed: ", curl_easy_strerror(res));
        throw std::runtime_error(std::string("Streaming request failed: ") + 
                               curl_easy_strerror(res));
    }
}

void CurlClient::set_progress_callback(ProgressCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_progress_callback = std::move(callback);
}

bool CurlClient::is_configured() const {
    return m_initialized;
}

std::string CurlClient::get_implementation_name() const {
    return "CURL";
}

// ClientFactory implementation
std::unique_ptr<ClientInterface> ClientFactory::create_default(
    const ClientConfig& config) {
    return create_curl_client(config);
}

std::unique_ptr<ClientInterface> ClientFactory::create_curl_client(
    const ClientConfig& config) {
    return std::make_unique<CurlClient>(config);
}

std::vector<std::string> ClientFactory::get_available_implementations() {
    std::vector<std::string> implementations;
    
    // CURL is always available if this code compiles
    implementations.push_back("CURL");
    
    // Future: Add other implementations
    // #ifdef _WIN32
    //     implementations.push_back("WinHTTP");
    // #endif
    
    return implementations;
}

} // namespace cql::http
