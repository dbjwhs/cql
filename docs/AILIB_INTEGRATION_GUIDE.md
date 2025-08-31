# AILib Integration Guide

A practical guide for developers integrating with AILib, the modern C++ AI provider library within CQL.

## Quick Start

### 1. Include AILib Headers

```cpp
#include "ailib/providers/factory.hpp"
#include "ailib/core/config.hpp"
// Additional headers as needed:
// #include "ailib/providers/anthropic.hpp"  
// #include "ailib/http/client.hpp"
// #include "ailib/auth/secure_store.hpp"
```

### 2. Basic Configuration

```cpp
// Create configuration
cql::Config config;
config.set_api_key("anthropic", "your-api-key-here");
config.set_model("anthropic", "claude-3-sonnet-20240229");
config.set_temperature(0.7);
config.set_max_tokens(1000);

// Create provider
auto& factory = cql::ProviderFactory::get_instance();
auto provider = factory.create_provider("anthropic", config);

// Verify configuration
if (!provider->is_configured()) {
    throw std::runtime_error("Provider not properly configured");
}
```

### 3. Simple Request

```cpp
// Create request
cql::ProviderRequest request;
request.prompt = "Write a simple C++ hello world program";
request.max_tokens = 200;

// Send request
auto response = provider->send_request(request);

// Handle response
if (response.is_success()) {
    std::cout << "Generated code:\n" << response.content << std::endl;
} else {
    std::cerr << "Error: " << response.error_message.value_or("Unknown error") << std::endl;
}
```

## Configuration Management

### Secure API Key Storage

```cpp
#include "ailib/auth/secure_store.hpp"

// Using SecureString for API keys
cql::SecureString api_key("your-sensitive-key");
config.set_api_key("anthropic", api_key.data());

// Load from environment with security
auto env_key = cql::secure_getenv("ANTHROPIC_API_KEY");
if (!env_key.empty()) {
    config.set_api_key("anthropic", env_key.data());
}
```

### Configuration Profiles

```cpp
// Development configuration
cql::Config dev_config;
dev_config.set_api_key("anthropic", "dev-key");
dev_config.set_temperature(0.9);  // More creative
dev_config.set_max_tokens(500);

// Production configuration  
cql::Config prod_config;
prod_config.set_api_key("anthropic", "prod-key");
prod_config.set_temperature(0.1);  // More deterministic
prod_config.set_max_tokens(2000);
prod_config.set_timeout(std::chrono::seconds(60));
```

### Multiple Provider Setup

```cpp
// Configure multiple providers
cql::Config multi_config;
multi_config.set_api_key("anthropic", "anthropic-key");
multi_config.set_model("anthropic", "claude-3-sonnet-20240229");

// Future: OpenAI configuration
// multi_config.set_api_key("openai", "openai-key");
// multi_config.set_model("openai", "gpt-4-turbo");

// Set fallback chain
multi_config.set_fallback_chain({"anthropic"});  // Future: {"anthropic", "openai"}
```

## Advanced Request Features

### Retry Logic with Exponential Backoff

```cpp
cql::ProviderRequest request;
request.prompt = "Your prompt here";

// Configure retry policy
request.retry_policy.max_retries = 5;
request.retry_policy.initial_delay = std::chrono::milliseconds(100);
request.retry_policy.backoff_multiplier = 2.0;
request.retry_policy.max_delay = std::chrono::seconds(30);
request.retry_policy.enable_jitter = true;  // Prevents thundering herd

auto response = provider->send_request(request);
```

### System Messages and Conversations

```cpp
cql::ProviderRequest conversation;
conversation.system_prompt = "You are a helpful C++ programming assistant.";
conversation.messages = {
    {"user", "How do I use smart pointers in C++?"},
    {"assistant", "Smart pointers in C++ provide automatic memory management..."},
    {"user", "Show me an example with unique_ptr"}
};
conversation.max_tokens = 500;

auto response = provider->send_request(conversation);
```

### Asynchronous Requests

```cpp
// Async request
auto future = provider->send_async(request);

// Do other work...
process_other_tasks();

// Get result when ready
auto response = future.get();
if (response.is_success()) {
    handle_response(response);
}
```

## HTTP Client Integration

### Direct HTTP Client Usage

```cpp
#include "ailib/http/client.hpp"

// Create HTTP client with custom config
cql::http::ClientConfig http_config;
http_config.default_timeout = std::chrono::seconds(30);
http_config.max_redirects = 5;
http_config.verify_ssl = true;
http_config.enable_compression = true;

auto client = cql::http::ClientFactory::create_curl_client(http_config);

// Make custom HTTP request
cql::http::Request http_request;
http_request.url = "https://api.anthropic.com/v1/messages";
http_request.method = "POST";
http_request.headers["Content-Type"] = "application/json";
http_request.headers["Authorization"] = "Bearer " + api_key;
http_request.body = request_json;

auto http_response = client->send(http_request);
```

### Progress Callbacks

```cpp
// Set progress callback for large requests
client->set_progress_callback([](size_t received, size_t total) {
    if (total > 0) {
        double progress = (double)received / total * 100.0;
        std::cout << "Progress: " << std::fixed << std::setprecision(1) 
                  << progress << "%" << std::endl;
    }
});
```

## Error Handling

### Comprehensive Error Checking

```cpp
auto response = provider->send_request(request);

if (!response.is_success()) {
    // Check error type
    if (response.is_client_error()) {
        std::cerr << "Client error (4xx): " << response.status_code << std::endl;
        // Handle authentication, bad request, etc.
    } else if (response.is_server_error()) {
        std::cerr << "Server error (5xx): " << response.status_code << std::endl;
        // Handle server downtime, rate limits, etc.
    }
    
    // Get detailed error information
    if (response.error_message.has_value()) {
        std::cerr << "Error details: " << response.error_message.value() << std::endl;
    }
    
    return false;
}
```

### Retry Strategy Based on Error Type

```cpp
bool should_retry(const cql::ProviderResponse& response) {
    // Retry on server errors and rate limits
    if (response.status_code >= 500 || response.status_code == 429) {
        return true;
    }
    
    // Don't retry on client errors
    if (response.status_code >= 400 && response.status_code < 500) {
        return false;
    }
    
    // Retry on network timeouts
    return !response.is_success();
}
```

## Testing Integration

### Mock Provider for Testing

```cpp
// Unit testing with mock responses
class MockProvider : public cql::AIProvider {
public:
    cql::ProviderResponse send_request(const cql::ProviderRequest& request) override {
        cql::ProviderResponse response;
        response.content = "Mock response for: " + request.prompt;
        response.status_code = 200;
        return response;
    }
    
    // Implement other required methods...
    std::string get_provider_name() const override { return "Mock"; }
    bool is_configured() const override { return true; }
};

// Use in tests
TEST(AILibTest, BasicIntegration) {
    auto mock_provider = std::make_unique<MockProvider>();
    cql::ProviderRequest request;
    request.prompt = "test prompt";
    
    auto response = mock_provider->send_request(request);
    EXPECT_TRUE(response.is_success());
    EXPECT_EQ(response.content, "Mock response for: test prompt");
}
```

## Performance Optimization

### Connection Reuse

```cpp
// Reuse provider instances for better performance
class AIService {
private:
    std::unique_ptr<cql::AIProvider> m_provider;
    
public:
    AIService(const cql::Config& config) {
        auto& factory = cql::ProviderFactory::get_instance();
        m_provider = factory.create_provider("anthropic", config);
    }
    
    cql::ProviderResponse generate(const std::string& prompt) {
        cql::ProviderRequest request;
        request.prompt = prompt;
        return m_provider->send_request(request);
    }
};
```

### Cost Estimation

```cpp
// Estimate costs before making requests
auto estimated_cost = provider->estimate_cost(request);
if (estimated_cost.has_value()) {
    std::cout << "Estimated cost: $" << std::fixed << std::setprecision(4) 
              << estimated_cost.value() << std::endl;
    
    // Ask user confirmation for expensive requests
    if (estimated_cost.value() > 0.10) {
        if (!confirm_expensive_request()) {
            return;
        }
    }
}
```

## Security Best Practices

### API Key Management

```cpp
// DO: Use environment variables
auto api_key = cql::secure_getenv("ANTHROPIC_API_KEY");
if (api_key.empty()) {
    throw std::runtime_error("ANTHROPIC_API_KEY environment variable not set");
}

// DO: Use SecureString for sensitive data
cql::SecureString secure_key(api_key.data());
config.set_api_key("anthropic", secure_key.data());

// DON'T: Hardcode API keys in source
// config.set_api_key("anthropic", "sk-hardcoded-key-bad");  // NEVER DO THIS
```

### Input Validation

```cpp
bool validate_request(const cql::ProviderRequest& request) {
    // Validate prompt length
    if (request.prompt.length() > 100000) {  // Reasonable limit
        std::cerr << "Prompt too long" << std::endl;
        return false;
    }
    
    // Validate parameters
    if (request.temperature < 0.0 || request.temperature > 2.0) {
        std::cerr << "Invalid temperature value" << std::endl;
        return false;
    }
    
    if (request.max_tokens <= 0 || request.max_tokens > 8192) {
        std::cerr << "Invalid max_tokens value" << std::endl;
        return false;
    }
    
    return true;
}
```

## Build Integration

### CMakeLists.txt Example

```cmake
# Your project's CMakeLists.txt
find_package(CURL REQUIRED)

# Include AILib headers
target_include_directories(your_target PRIVATE 
    ${CMAKE_SOURCE_DIR}/lib/ailib/include
)

# Link with CQL (includes AILib)
target_link_libraries(your_target PRIVATE cql_lib)
```

### Compilation Example

```bash
# Compile with AILib
g++ -std=c++20 -I./lib/ailib/include \
    your_code.cpp -lcurl -pthread \
    -o your_program

# Or use CMake (recommended)
mkdir build && cd build
cmake .. && make
```

## Troubleshooting

### Common Issues

1. **Provider Not Configured**
   ```cpp
   if (!provider->is_configured()) {
       auto errors = config.get_validation_errors();
       for (const auto& error : errors) {
           std::cerr << "Config error: " << error << std::endl;
       }
   }
   ```

2. **Network Timeouts**
   ```cpp
   config.set_timeout(std::chrono::seconds(120));  // Increase timeout
   request.retry_policy.max_retries = 5;  // More retries
   ```

3. **Rate Limiting**
   ```cpp
   // Handle 429 responses with exponential backoff
   request.retry_policy.initial_delay = std::chrono::seconds(1);
   request.retry_policy.backoff_multiplier = 3.0;
   ```

### Debug Logging

```cpp
#include "ailib/core/logger.hpp"

// Enable debug logging
cql::Logger::getInstance().set_log_level(cql::LogLevel::DEBUG);

// Log requests and responses
cql::Logger::getInstance().log(cql::LogLevel::DEBUG, 
    "Sending request to provider: ", provider->get_provider_name());
```

---

## Next Steps

- **Check out the [API Documentation](api_documentation.md)** for complete interface reference
- **Read the [AILib Design Specification](CQL_AILIB_DESIGN_SPECIFICATION.md)** for architectural details  
- **See [examples/](../examples/)** for complete working examples
- **Run the test suite**: `build/cql_test --gtest_filter="*ailib*"` for AILib-specific tests

AILib provides a solid foundation for C++ AI integration. As more providers are added, the same patterns and interfaces will work seamlessly across all supported AI services.
