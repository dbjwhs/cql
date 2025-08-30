# CQL API Client Specification

This document provides detailed specifications for the API client that will connect the CQL compiler to Anthropic's Claude API.

## Class Structure

```cpp
namespace cql {

class ApiClient {
public:
    // Constructor and Destructor
    explicit ApiClient(const Config& config);
    ~ApiClient();
    
    // Main API Methods
    ApiResponse submit_query(const std::string& query);
    ApiResponse submit_query_async(const std::string& query, std::function<void(ApiResponse)> callback);
    
    // Configuration Methods
    void set_model(const std::string& model);
    void set_api_key(const std::string& api_key);
    void set_timeout(int timeout_seconds);
    void set_max_retries(int max_retries);
    
    // Status Methods
    bool is_connected() const;
    ApiClientStatus get_status() const;
    std::string get_last_error() const;
    
private:
    // Private Implementation Details
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

struct ApiResponse {
    bool success;
    int status_code;
    std::string raw_response;
    std::vector<GeneratedFile> generated_files;
    std::string error_message;
    
    // Helper methods for processing the response
    bool has_error() const;
    std::string get_main_content() const;
};

struct GeneratedFile {
    std::string filename;
    std::string language;
    std::string content;
    bool is_test;
};

enum class ApiClientStatus {
    Ready,
    Connecting,
    Processing,
    Error,
    RateLimited
};

} // namespace cql
```

## HTTP Client Implementation

The API client will use a lightweight HTTP library (such as libcurl or cpp-httplib) to communicate with the Anthropic API. The implementation should:

1. Support HTTPS connections
2. Handle request headers (API key, content type)
3. Support timeout controls
4. Provide detailed error information
5. Allow async operations

## Configuration Management

The API client will be configured through a `Config` class that loads settings from:

1. Environment variables (e.g., `CQL_API_KEY`)
2. Config file (~/.cql/config.json)
3. Command-line arguments

Priority will be given to command-line arguments over environment variables over config file settings.

```cpp
class Config {
public:
    static Config load_from_default_locations();
    static Config load_from_file(const std::string& filename);
    
    std::string get_api_key() const;
    std::string get_model() const;
    int get_timeout() const;
    int get_max_retries() const;
    std::string get_output_directory() const;
    bool should_overwrite_existing_files() const;
    bool should_create_missing_directories() const;
    
    // Setters
    void set_api_key(const std::string& api_key);
    void set_model(const std::string& model);
    // ... other setters
};
```

## API Request Format

Requests to the Anthropic API should follow this format:

```json
{
  "model": "claude-3-opus-20240229",
  "max_tokens": 100000,
  "messages": [
    {
      "role": "user",
      "content": "compiled CQL query goes here"
    }
  ]
}
```

The compiled CQL query will be the content of the user message.

## Response Processing

The `ApiResponse` structure will include:

1. The raw API response from Claude
2. A processed list of `GeneratedFile` objects with code extracted from the response
3. Error information if the request failed

Code extraction will:
1. Parse Markdown code blocks from the response
2. Identify language from code block tags
3. Group related code blocks for multi-file responses
4. Extract filenames from comments or context

## Error Handling

The API client should handle the following error scenarios:

1. Network connection failures
2. API authentication failures (invalid API key)
3. Rate limiting and quota issues
4. Timeout errors
5. Malformed responses

Each error should provide:
1. A clear error code
2. A human-readable error message
3. Guidance on how to resolve the issue (when possible)

## Retries and Rate Limiting

The client will implement:

1. Exponential backoff for retries
2. Proper handling of rate limit headers
3. Configurable maximum retry attempts
4. Timeout controls

## Usage Example

```cpp
// Create config from default locations (env vars, config file)
cql::Config config = cql::Config::load_from_default_locations();

// Override specific settings if needed
config.set_model("claude-3-opus-20240229");
config.set_timeout(60);

// Create API client
cql::ApiClient api_client(config);
Logger::getInstance().log(LogLevel::INFO, "Created API client with model: ", config.get_model());

// Submit a query
std::string compiled_query = compiler.get_compiled_query();
Logger::getInstance().log(LogLevel::INFO, "Submitting query to Claude API");
cql::ApiResponse response = api_client.submit_query(compiled_query);

// Process response
if (response.success) {
    // Process generated files
    Logger::getInstance().log(LogLevel::INFO, "API request successful, processing ", response.generated_files.size(), " files");
    
    for (const auto& file : response.generated_files) {
        Logger::getInstance().log(LogLevel::INFO, "Processing generated file: ", file.filename);
        
        // Save file to disk
        save_generated_file(file, config.get_output_directory());
    }
} else {
    // Handle error
    Logger::getInstance().log(LogLevel::ERROR, "API request failed: ", response.error_message);
}
```

## Extension Points

The API client design includes several extension points for future enhancements:

1. **Streaming responses**: Support for streaming large responses
2. **Multi-turn conversations**: Support for maintaining conversation context over multiple interactions
3. **Model tuning**: Support for model-specific parameters like temperature and top-p
4. **Batched requests**: Support for sending multiple queries in a batch
5. **Response caching**: Support for caching API responses to reduce API usage

## Logging Implementation

The API client will use the project's custom logger for all logging operations:

```cpp
// Example logging implementation
void send_request(const std::string& endpoint, const std::string& payload) {
    Logger::getInstance().log(LogLevel::INFO, "Sending request to endpoint: ", endpoint);
    
    try {
        // Perform HTTP request
        // ...
        
        Logger::getInstance().log(LogLevel::INFO, "Request successful");
    } catch (const NetworkException& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Network error: ", e.what(), 
                                 " (Code: ", e.error_code(), ")");
        throw;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Request failed: ", e.what());
        throw;
    }
}

// Response handling with logging
void process_response(const HttpResponse& http_response) {
    Logger::getInstance().log(LogLevel::INFO, "Processing response with status code: ", http_response.status_code);
    
    if (http_response.status_code >= 400) {
        Logger::getInstance().log(LogLevel::ERROR, "API error response: ", http_response.body);
        // Handle error
    } else {
        Logger::getInstance().log(LogLevel::INFO, "API response successful, content length: ", http_response.body.size());
        // Process success
    }
}
```

## Security Considerations

The API client implementation must address these security concerns:

1. **API key protection**: Never log or expose API keys
2. **Secure storage**: Store API keys securely, possibly using platform credential stores
3. **TLS verification**: Always verify TLS certificates for API connections
4. **Input sanitization**: Validate and sanitize all inputs to prevent injection attacks
5. **Output validation**: Validate response content before execution or storage

## Performance Considerations

The implementation should:

1. Minimize memory usage for large responses
2. Support async operations to prevent blocking
3. Implement connection pooling for efficiency
4. Consider compression for large request/response payloads
5. Provide progress indicators for long-running operations

## Testing Strategy

The API client should be tested with:

1. Unit tests with mocked API responses
2. Integration tests with a sandbox API environment
3. Error case testing (network errors, API errors)
4. Performance testing for large queries/responses
5. Cross-platform compatibility tests

## Documentation Requirements

The API client should include:

1. Comprehensive class and method documentation with examples
2. Error code documentation with resolution steps
3. Configuration guide for different environments
4. Troubleshooting guide for common issues
5. Sample code for common operations
