// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/anthropic_provider.hpp"
#include "../../include/cql/json_utils.hpp"
#include "../../include/cql/project_utils.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <future>
#include <thread>

namespace cql {

namespace {

// Supported Claude models and their specifications
const std::map<std::string, std::pair<size_t, size_t>> CLAUDE_MODELS = {
    {"claude-3-opus-20240229", {200000, 4096}},      // context, max_output
    {"claude-3-sonnet-20240229", {200000, 4096}},
    {"claude-3-haiku-20240307", {200000, 4096}},
    {"claude-3-5-sonnet-20241022", {200000, 8192}},
    {"claude-3-5-haiku-20241022", {200000, 8192}},
    {"claude-3-opus", {200000, 4096}},               // Aliases
    {"claude-3-sonnet", {200000, 4096}},
    {"claude-3-haiku", {200000, 4096}},
    {"claude-3.5-sonnet", {200000, 8192}},
    {"claude-3.5-haiku", {200000, 8192}}
};

// Token pricing per model (input, output) per 1K tokens in USD
const std::map<std::string, std::pair<double, double>> CLAUDE_PRICING = {
    {"claude-3-opus-20240229", {0.015, 0.075}},
    {"claude-3-sonnet-20240229", {0.003, 0.015}},
    {"claude-3-haiku-20240307", {0.00025, 0.00125}},
    {"claude-3-5-sonnet-20241022", {0.003, 0.015}},
    {"claude-3-5-haiku-20241022", {0.001, 0.005}}
};

} // anonymous namespace

AnthropicProvider::AnthropicProvider(const Config& config) 
    : m_config(config) {
    
    Logger::getInstance().log(LogLevel::DEBUG, "AnthropicProvider constructor called");
    
    // Create HTTP client with Anthropic-optimized settings
    http::ClientConfig http_config;
    http_config.default_timeout = std::chrono::seconds(120); // Longer timeout for AI requests
    http_config.verify_ssl = true;
    http_config.enable_compression = true;
    
    Logger::getInstance().log(LogLevel::DEBUG, "HTTP client config created with timeout: ", 
        http_config.default_timeout.count(), "s");
    
    // Set default headers
    auto headers = create_headers();
    Logger::getInstance().log(LogLevel::DEBUG, "Created ", headers.size(), " default headers");
    for (const auto& [key, value] : headers) {
        http_config.default_headers[key] = value;
        Logger::getInstance().log(LogLevel::DEBUG, "Header set: ", key, " = ", 
            (key == "x-api-key" ? "[REDACTED]" : value));
    }
    
    m_http_client = http::ClientFactory::create_default(http_config);
    Logger::getInstance().log(LogLevel::DEBUG, "HTTP client created successfully");
    
    Logger::getInstance().log(LogLevel::INFO, 
        "AnthropicProvider initialized with base URL: ", 
        m_config.get_base_url("anthropic").value_or(BASE_URL));
}

ProviderResponse AnthropicProvider::generate(const ProviderRequest& request) {
    Logger::getInstance().log(LogLevel::INFO, "Generating response with model: ", request.model);
    Logger::getInstance().log(LogLevel::DEBUG, "Request details - prompt length: ", request.prompt.length(),
        ", max_tokens: ", request.max_tokens, ", temperature: ", request.temperature);
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Convert to Anthropic API format
        Logger::getInstance().log(LogLevel::DEBUG, "Converting request to Anthropic API format");
        nlohmann::json api_request = convert_request(request);
        std::string request_body = api_request.dump();
        Logger::getInstance().log(LogLevel::DEBUG, "Request body size: ", request_body.length(), " bytes");
        
        // Prepare HTTP request
        std::string base_url = m_config.get_base_url("anthropic").value_or(BASE_URL);
        Logger::getInstance().log(LogLevel::DEBUG, "Preparing HTTP request to: ", base_url, "/v1/messages");
        http::Request http_request;
        http_request.url = base_url + "/v1/messages";
        http_request.method = "POST";
        http_request.body = request_body;
        http_request.headers = create_headers();
        http_request.timeout = std::chrono::seconds(120);
        
        // Configure retry policy from config
        http_request.retry_policy.max_retries = m_config.get_max_retries("anthropic");
        Logger::getInstance().log(LogLevel::DEBUG, "HTTP request prepared with ", http_request.headers.size(), 
                                " headers and max_retries: ", http_request.retry_policy.max_retries);
        
        // Send request
        Logger::getInstance().log(LogLevel::DEBUG, "Sending HTTP request to Anthropic API");
        http::Response response = m_http_client->send(http_request);
        
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        Logger::getInstance().log(LogLevel::DEBUG, "HTTP response received in ", latency.count(), "ms, status: ", response.status_code);
        
        // Handle HTTP errors
        if (!response.is_success()) {
            Logger::getInstance().log(LogLevel::DEBUG, "HTTP error detected, response body: ", 
                response.body.substr(0, 200), (response.body.length() > 200 ? "..." : ""));
            
            ProviderResponse error_response;
            error_response.success = false;
            error_response.http_status = response.status_code;
            error_response.latency = latency;
            
            if (response.is_client_error()) {
                error_response.error_message = "Client error: " + std::to_string(response.status_code);
            } else if (response.is_server_error()) {
                error_response.error_message = "Server error: " + std::to_string(response.status_code);
            } else {
                error_response.error_message = "HTTP error: " + std::to_string(response.status_code);
            }
            
            Logger::getInstance().log(LogLevel::ERROR, error_response.error_message.value());
            return error_response;
        }
        
        // Parse JSON response
        Logger::getInstance().log(LogLevel::DEBUG, "Parsing JSON response, body size: ", response.body.length(), " bytes");
        nlohmann::json json_response = nlohmann::json::parse(response.body);
        Logger::getInstance().log(LogLevel::DEBUG, "JSON parsed successfully, calling parse_response");
        return parse_response(json_response, latency);
        
    } catch (const nlohmann::json::exception& e) {
        ProviderResponse error_response;
        error_response.success = false;
        error_response.error_message = "JSON parsing error: " + std::string(e.what());
        error_response.latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        Logger::getInstance().log(LogLevel::ERROR, error_response.error_message.value());
        return error_response;
    } catch (const std::exception& e) {
        ProviderResponse error_response;
        error_response.success = false;
        error_response.error_message = "Request error: " + std::string(e.what());
        error_response.latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        Logger::getInstance().log(LogLevel::ERROR, error_response.error_message.value());
        return error_response;
    }
}

std::future<ProviderResponse> AnthropicProvider::generate_async(const ProviderRequest& request) {
    Logger::getInstance().log(LogLevel::DEBUG, "Starting async generation for model: ", request.model);
    return std::async(std::launch::async, [this, request]() {
        Logger::getInstance().log(LogLevel::DEBUG, "Async thread started, calling generate");
        auto result = generate(request);
        Logger::getInstance().log(LogLevel::DEBUG, "Async generation completed, success: ", result.success);
        return result;
    });
}

void AnthropicProvider::generate_stream(const ProviderRequest& request, StreamingCallback callback) {
    Logger::getInstance().log(LogLevel::INFO, "Starting streaming response with model: ", request.model);
    Logger::getInstance().log(LogLevel::DEBUG, "Streaming request details - prompt length: ", request.prompt.length(),
        ", max_tokens: ", request.max_tokens);
    
    try {
        // Convert to Anthropic API format with streaming enabled
        Logger::getInstance().log(LogLevel::DEBUG, "Converting request to streaming Anthropic API format");
        nlohmann::json api_request = convert_request(request);
        api_request["stream"] = true;
        std::string request_body = api_request.dump();
        Logger::getInstance().log(LogLevel::DEBUG, "Streaming request body size: ", request_body.length(), " bytes");
        
        // Prepare HTTP request
        std::string base_url = m_config.get_base_url("anthropic").value_or(BASE_URL);
        Logger::getInstance().log(LogLevel::DEBUG, "Preparing streaming HTTP request to: ", base_url, "/v1/messages");
        http::Request http_request;
        http_request.url = base_url + "/v1/messages";
        http_request.method = "POST";
        http_request.body = request_body;
        http_request.headers = create_headers();
        http_request.timeout = std::chrono::seconds(300); // Longer timeout for streaming
        
        // Configure retry policy from config (usually fewer retries for streaming)
        http_request.retry_policy.max_retries = std::min(1, m_config.get_max_retries("anthropic"));
        Logger::getInstance().log(LogLevel::DEBUG, "Streaming HTTP request prepared with timeout: 300s and max_retries: ", 
                                http_request.retry_policy.max_retries);
        
        // Send streaming request
        Logger::getInstance().log(LogLevel::DEBUG, "Starting streaming HTTP request");
        m_http_client->send_stream(http_request, [this, callback](const std::string& chunk) {
            Logger::getInstance().log(LogLevel::DEBUG, "Received streaming chunk, size: ", chunk.length(), " bytes");
            auto streaming_chunk = parse_stream_chunk(chunk);
            if (streaming_chunk) {
                Logger::getInstance().log(LogLevel::DEBUG, "Parsed streaming chunk, is_final: ", streaming_chunk->is_final);
                callback(streaming_chunk.value());
            } else {
                Logger::getInstance().log(LogLevel::DEBUG, "Skipping invalid streaming chunk");
            }
        });
        Logger::getInstance().log(LogLevel::DEBUG, "Streaming request completed");
        
    } catch (const std::exception& e) {
        // Send error chunk
        StreamingChunk error_chunk;
        error_chunk.error = "Streaming error: " + std::string(e.what());
        error_chunk.is_final = true;
        callback(error_chunk);
        
        Logger::getInstance().log(LogLevel::ERROR, error_chunk.error.value());
    }
}

std::string AnthropicProvider::get_provider_name() const {
    return "Anthropic";
}

ProviderCapabilities AnthropicProvider::get_capabilities() const {
    ProviderCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_functions = false; // Not yet supported in this implementation
    caps.supports_vision = true;     // Claude 3 supports vision
    caps.supports_async = true;
    
    // Get available models
    for (const auto& [model, _] : CLAUDE_MODELS) {
        caps.available_models.push_back(model);
    }
    
    // Use the largest context window available
    caps.max_context_length = 200000;
    caps.max_output_tokens = 8192;
    
    return caps;
}

bool AnthropicProvider::is_configured() const {
    std::string api_key = m_config.get_api_key("anthropic");
    bool configured = !api_key.empty() && api_key.length() >= 30;
    Logger::getInstance().log(LogLevel::DEBUG, "Configuration check - API key present: ", !api_key.empty(),
        ", length valid: ", (api_key.length() >= 30), ", configured: ", configured);
    return configured; // Basic API key validation
}

bool AnthropicProvider::validate_model(const std::string& model) const {
    bool valid = CLAUDE_MODELS.find(model) != CLAUDE_MODELS.end();
    Logger::getInstance().log(LogLevel::DEBUG, "Model validation for '", model, "': ", (valid ? "valid" : "invalid"));
    return valid;
}

std::optional<double> AnthropicProvider::estimate_cost(const ProviderRequest& request) const {
    Logger::getInstance().log(LogLevel::DEBUG, "Estimating cost for model: ", request.model);
    
    auto pricing_it = CLAUDE_PRICING.find(request.model);
    if (pricing_it == CLAUDE_PRICING.end()) {
        Logger::getInstance().log(LogLevel::DEBUG, "No pricing data found for model: ", request.model);
        return std::nullopt; // Unknown model
    }
    
    const auto& [input_price, output_price] = pricing_it->second;
    Logger::getInstance().log(LogLevel::DEBUG, "Pricing found - input: $", input_price, "/1K tokens, output: $", output_price, "/1K tokens");
    
    // Rough estimate: 4 chars per token for input
    double estimated_input_tokens = request.prompt.length() / 4.0;
    double estimated_output_tokens = request.max_tokens;
    
    // Add system prompt and messages if present
    if (request.system_prompt) {
        estimated_input_tokens += request.system_prompt->length() / 4.0;
        Logger::getInstance().log(LogLevel::DEBUG, "Added system prompt tokens: ", request.system_prompt->length() / 4.0);
    }
    
    for (const auto& [role, content] : request.messages) {
        estimated_input_tokens += content.length() / 4.0;
        Logger::getInstance().log(LogLevel::DEBUG, "Added message tokens for ", role, ": ", content.length() / 4.0);
    }
    
    // Calculate cost in USD
    double input_cost = (estimated_input_tokens / 1000.0) * input_price;
    double output_cost = (estimated_output_tokens / 1000.0) * output_price;
    double total_cost = input_cost + output_cost;
    
    Logger::getInstance().log(LogLevel::DEBUG, "Cost calculation - input tokens: ", estimated_input_tokens,
        ", output tokens: ", estimated_output_tokens, ", total cost: $", total_cost);
    
    return total_cost;
}

nlohmann::json AnthropicProvider::convert_request(const ProviderRequest& request) const {
    Logger::getInstance().log(LogLevel::DEBUG, "Converting ProviderRequest to Anthropic API format");
    nlohmann::json api_request = nlohmann::json::object();
    
    // Required fields
    api_request["model"] = request.model;
    api_request["max_tokens"] = request.max_tokens;
    Logger::getInstance().log(LogLevel::DEBUG, "Set required fields - model: ", request.model, ", max_tokens: ", request.max_tokens);
    
    // Messages array - convert from simple prompt or use provided messages
    nlohmann::json messages = nlohmann::json::array();
    
    if (!request.messages.empty()) {
        Logger::getInstance().log(LogLevel::DEBUG, "Processing ", request.messages.size(), " existing messages");
        // Use provided conversation history
        for (const auto& [role, content] : request.messages) {
            nlohmann::json message;
            message["role"] = role;
            message["content"] = content;
            messages.push_back(message);
            Logger::getInstance().log(LogLevel::DEBUG, "Added message - role: ", role, ", content length: ", content.length());
        }
        
        // Add current prompt as final user message if not already a user message
        if (request.messages.empty() || request.messages.back().first != "user") {
            nlohmann::json user_message;
            user_message["role"] = "user";
            user_message["content"] = request.prompt;
            messages.push_back(user_message);
            Logger::getInstance().log(LogLevel::DEBUG, "Added final user message with prompt length: ", request.prompt.length());
        }
    } else {
        Logger::getInstance().log(LogLevel::DEBUG, "No existing messages, creating single user message");
        // Simple prompt - create single user message
        nlohmann::json user_message;
        user_message["role"] = "user";
        user_message["content"] = request.prompt;
        messages.push_back(user_message);
        Logger::getInstance().log(LogLevel::DEBUG, "Created user message with prompt length: ", request.prompt.length());
    }
    
    api_request["messages"] = messages;
    Logger::getInstance().log(LogLevel::DEBUG, "Set messages array with ", messages.size(), " messages");
    
    // Optional parameters
    if (request.temperature != 0.7) { // Only include if not default
        api_request["temperature"] = request.temperature;
        Logger::getInstance().log(LogLevel::DEBUG, "Set temperature: ", request.temperature);
    }
    
    if (request.top_p) {
        api_request["top_p"] = request.top_p.value();
        Logger::getInstance().log(LogLevel::DEBUG, "Set top_p: ", request.top_p.value());
    }
    
    if (request.system_prompt) {
        api_request["system"] = request.system_prompt.value();
        Logger::getInstance().log(LogLevel::DEBUG, "Set system prompt, length: ", request.system_prompt->length());
    }
    
    // Add provider-specific metadata
    Logger::getInstance().log(LogLevel::DEBUG, "Processing ", request.metadata.size(), " metadata entries");
    for (const auto& [key, value] : request.metadata) {
        if (key.starts_with("anthropic_")) {
            std::string api_key = key.substr(10); // Remove "anthropic_" prefix
            api_request[api_key] = value;
            Logger::getInstance().log(LogLevel::DEBUG, "Added metadata: ", api_key, " = ", value);
        }
    }
    
    Logger::getInstance().log(LogLevel::DEBUG, "Request conversion completed");
    return api_request;
}

ProviderResponse AnthropicProvider::parse_response(const nlohmann::json& json_response, 
                                                  std::chrono::milliseconds latency) const {
    Logger::getInstance().log(LogLevel::DEBUG, "Parsing Anthropic API response");
    ProviderResponse response;
    response.latency = latency;
    
    try {
        // Check for API errors
        if (json_response.contains("error")) {
            Logger::getInstance().log(LogLevel::DEBUG, "API error detected in response");
            response.success = false;
            const auto& error = json_response["error"];
            
            if (error.contains("message")) {
                response.error_message = error["message"].get<std::string>();
                Logger::getInstance().log(LogLevel::DEBUG, "Error message: ", response.error_message.value());
            } else {
                response.error_message = "Unknown API error";
                Logger::getInstance().log(LogLevel::DEBUG, "No error message found in response");
            }
            
            if (error.contains("type")) {
                response.metadata["error_type"] = error["type"].get<std::string>();
                Logger::getInstance().log(LogLevel::DEBUG, "Error type: ", error["type"].get<std::string>());
            }
            
            return response;
        }
        
        // Parse successful response
        Logger::getInstance().log(LogLevel::DEBUG, "Parsing successful API response");
        response.success = true;
        response.model_used = json_response.value("model", "unknown");
        Logger::getInstance().log(LogLevel::DEBUG, "Model used: ", response.model_used);
        
        // Extract content from content array
        if (json_response.contains("content") && json_response["content"].is_array()) {
            const auto& content_array = json_response["content"];
            Logger::getInstance().log(LogLevel::DEBUG, "Content array contains ", content_array.size(), " items");
            std::ostringstream content_stream;
            
            for (const auto& content_item : content_array) {
                if (content_item.contains("text")) {
                    std::string text_content = content_item["text"].get<std::string>();
                    content_stream << text_content;
                    Logger::getInstance().log(LogLevel::DEBUG, "Added text content, length: ", text_content.length());
                }
            }
            
            response.content = content_stream.str();
            Logger::getInstance().log(LogLevel::DEBUG, "Total content length: ", response.content.length());
        } else {
            Logger::getInstance().log(LogLevel::DEBUG, "No content array found in response");
        }
        
        // Extract usage information
        if (json_response.contains("usage")) {
            const auto& usage = json_response["usage"];
            response.prompt_tokens = usage.value("input_tokens", 0);
            response.completion_tokens = usage.value("output_tokens", 0);
            response.tokens_used = response.prompt_tokens + response.completion_tokens;
            Logger::getInstance().log(LogLevel::DEBUG, "Usage - input tokens: ", response.prompt_tokens,
                ", output tokens: ", response.completion_tokens, ", total: ", response.tokens_used);
        } else {
            Logger::getInstance().log(LogLevel::DEBUG, "No usage information found in response");
        }
        
        // Extract additional metadata
        response.metadata["id"] = json_response.value("id", "");
        response.metadata["stop_reason"] = json_response.value("stop_reason", "");
        response.metadata["stop_sequence"] = json_response.value("stop_sequence", "");
        Logger::getInstance().log(LogLevel::DEBUG, "Extracted metadata - id: ", response.metadata["id"],
            ", stop_reason: ", response.metadata["stop_reason"]);
        
    } catch (const nlohmann::json::exception& e) {
        response.success = false;
        response.error_message = "Response parsing error: " + std::string(e.what());
        
        Logger::getInstance().log(LogLevel::ERROR, response.error_message.value());
    }
    
    return response;
}

std::map<std::string, std::string> AnthropicProvider::create_headers() const {
    Logger::getInstance().log(LogLevel::DEBUG, "Creating HTTP headers for Anthropic API");
    std::map<std::string, std::string> headers;
    
    headers["Content-Type"] = "application/json";
    headers["x-api-key"] = m_config.get_api_key("anthropic");
    headers["anthropic-version"] = API_VERSION;
    headers["User-Agent"] = "CQL-AnthropicProvider/1.0";
    
    Logger::getInstance().log(LogLevel::DEBUG, "Created headers - Content-Type, x-api-key, anthropic-version: ",
        API_VERSION, ", User-Agent");
    
    return headers;
}

std::optional<StreamingChunk> AnthropicProvider::parse_stream_chunk(const std::string& chunk) const {
    Logger::getInstance().log(LogLevel::DEBUG, "Parsing streaming chunk, size: ", chunk.length(), " bytes");
    
    // Anthropic SSE format: "data: {json}\n\n"
    if (!chunk.starts_with("data: ")) {
        Logger::getInstance().log(LogLevel::DEBUG, "Chunk does not start with 'data: ', skipping");
        return std::nullopt;
    }
    
    try {
        std::string json_str = chunk.substr(6); // Remove "data: " prefix
        Logger::getInstance().log(LogLevel::DEBUG, "Extracted JSON string, length: ", json_str.length());
        
        // Remove trailing newlines
        while (!json_str.empty() && (json_str.back() == '\n' || json_str.back() == '\r')) {
            json_str.pop_back();
        }
        
        // Handle special SSE events
        if (json_str == "[DONE]") {
            Logger::getInstance().log(LogLevel::DEBUG, "Received [DONE] event, marking as final chunk");
            StreamingChunk final_chunk;
            final_chunk.is_final = true;
            return final_chunk;
        }
        
        nlohmann::json json_chunk = nlohmann::json::parse(json_str);
        Logger::getInstance().log(LogLevel::DEBUG, "Parsed streaming JSON chunk successfully");
        StreamingChunk streaming_chunk;
        
        // Check for errors
        if (json_chunk.contains("error")) {
            streaming_chunk.error = json_chunk["error"]["message"].get<std::string>();
            streaming_chunk.is_final = true;
            Logger::getInstance().log(LogLevel::DEBUG, "Streaming error: ", streaming_chunk.error.value());
            return streaming_chunk;
        }
        
        // Extract content delta
        if (json_chunk.contains("delta") && json_chunk["delta"].contains("text")) {
            streaming_chunk.content = json_chunk["delta"]["text"].get<std::string>();
            Logger::getInstance().log(LogLevel::DEBUG, "Extracted content delta, length: ", streaming_chunk.content.length());
        }
        
        // Check if this is the final chunk
        if (json_chunk.contains("type")) {
            std::string event_type = json_chunk["type"].get<std::string>();
            streaming_chunk.is_final = (event_type == "message_stop" || event_type == "content_block_stop");
            Logger::getInstance().log(LogLevel::DEBUG, "Event type: ", event_type, ", is_final: ", streaming_chunk.is_final);
        }
        
        return streaming_chunk;
        
    } catch (const nlohmann::json::exception& e) {
        StreamingChunk error_chunk;
        error_chunk.error = "Chunk parsing error: " + std::string(e.what());
        error_chunk.is_final = true;
        return error_chunk;
    }
}

} // namespace cql
