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
    
    // Create HTTP client with Anthropic-optimized settings
    http::ClientConfig http_config;
    http_config.default_timeout = std::chrono::seconds(120); // Longer timeout for AI requests
    http_config.verify_ssl = true;
    http_config.enable_compression = true;
    
    // Set default headers
    auto headers = create_headers();
    for (const auto& [key, value] : headers) {
        http_config.default_headers[key] = value;
    }
    
    m_http_client = http::ClientFactory::create_default(http_config);
    
    Logger::getInstance().log(LogLevel::INFO, 
        "AnthropicProvider initialized with base URL: ", 
        m_config.get_base_url("anthropic").value_or(BASE_URL));
}

ProviderResponse AnthropicProvider::generate(const ProviderRequest& request) {
    Logger::getInstance().log(LogLevel::INFO, "Generating response with model: ", request.model);
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Convert to Anthropic API format
        nlohmann::json api_request = convert_request(request);
        std::string request_body = api_request.dump();
        
        // Prepare HTTP request
        std::string base_url = m_config.get_base_url("anthropic").value_or(BASE_URL);
        http::Request http_request;
        http_request.url = base_url + "/v1/messages";
        http_request.method = "POST";
        http_request.body = request_body;
        http_request.headers = create_headers();
        http_request.timeout = std::chrono::seconds(120);
        
        // Send request
        http::Response response = m_http_client->send(http_request);
        
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Handle HTTP errors
        if (!response.is_success()) {
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
        nlohmann::json json_response = nlohmann::json::parse(response.body);
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
    return std::async(std::launch::async, [this, request]() {
        return generate(request);
    });
}

void AnthropicProvider::generate_stream(const ProviderRequest& request, StreamingCallback callback) {
    Logger::getInstance().log(LogLevel::INFO, "Starting streaming response with model: ", request.model);
    
    try {
        // Convert to Anthropic API format with streaming enabled
        nlohmann::json api_request = convert_request(request);
        api_request["stream"] = true;
        std::string request_body = api_request.dump();
        
        // Prepare HTTP request
        std::string base_url = m_config.get_base_url("anthropic").value_or(BASE_URL);
        http::Request http_request;
        http_request.url = base_url + "/v1/messages";
        http_request.method = "POST";
        http_request.body = request_body;
        http_request.headers = create_headers();
        http_request.timeout = std::chrono::seconds(300); // Longer timeout for streaming
        
        // Send streaming request
        m_http_client->send_stream(http_request, [this, callback](const std::string& chunk) {
            auto streaming_chunk = parse_stream_chunk(chunk);
            if (streaming_chunk) {
                callback(streaming_chunk.value());
            }
        });
        
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
    return !api_key.empty() && api_key.length() >= 30; // Basic API key validation
}

bool AnthropicProvider::validate_model(const std::string& model) const {
    return CLAUDE_MODELS.find(model) != CLAUDE_MODELS.end();
}

std::optional<double> AnthropicProvider::estimate_cost(const ProviderRequest& request) const {
    auto pricing_it = CLAUDE_PRICING.find(request.model);
    if (pricing_it == CLAUDE_PRICING.end()) {
        return std::nullopt; // Unknown model
    }
    
    const auto& [input_price, output_price] = pricing_it->second;
    
    // Rough estimate: 4 chars per token for input
    double estimated_input_tokens = request.prompt.length() / 4.0;
    double estimated_output_tokens = request.max_tokens;
    
    // Add system prompt and messages if present
    if (request.system_prompt) {
        estimated_input_tokens += request.system_prompt->length() / 4.0;
    }
    
    for (const auto& [role, content] : request.messages) {
        estimated_input_tokens += content.length() / 4.0;
    }
    
    // Calculate cost in USD
    double input_cost = (estimated_input_tokens / 1000.0) * input_price;
    double output_cost = (estimated_output_tokens / 1000.0) * output_price;
    
    return input_cost + output_cost;
}

nlohmann::json AnthropicProvider::convert_request(const ProviderRequest& request) const {
    nlohmann::json api_request = nlohmann::json::object();
    
    // Required fields
    api_request["model"] = request.model;
    api_request["max_tokens"] = request.max_tokens;
    
    // Messages array - convert from simple prompt or use provided messages
    nlohmann::json messages = nlohmann::json::array();
    
    if (!request.messages.empty()) {
        // Use provided conversation history
        for (const auto& [role, content] : request.messages) {
            nlohmann::json message;
            message["role"] = role;
            message["content"] = content;
            messages.push_back(message);
        }
        
        // Add current prompt as final user message if not already a user message
        if (request.messages.empty() || request.messages.back().first != "user") {
            nlohmann::json user_message;
            user_message["role"] = "user";
            user_message["content"] = request.prompt;
            messages.push_back(user_message);
        }
    } else {
        // Simple prompt - create single user message
        nlohmann::json user_message;
        user_message["role"] = "user";
        user_message["content"] = request.prompt;
        messages.push_back(user_message);
    }
    
    api_request["messages"] = messages;
    
    // Optional parameters
    if (request.temperature != 0.7) { // Only include if not default
        api_request["temperature"] = request.temperature;
    }
    
    if (request.top_p) {
        api_request["top_p"] = request.top_p.value();
    }
    
    if (request.system_prompt) {
        api_request["system"] = request.system_prompt.value();
    }
    
    // Add provider-specific metadata
    for (const auto& [key, value] : request.metadata) {
        if (key.starts_with("anthropic_")) {
            std::string api_key = key.substr(10); // Remove "anthropic_" prefix
            api_request[api_key] = value;
        }
    }
    
    return api_request;
}

ProviderResponse AnthropicProvider::parse_response(const nlohmann::json& json_response, 
                                                  std::chrono::milliseconds latency) const {
    ProviderResponse response;
    response.latency = latency;
    
    try {
        // Check for API errors
        if (json_response.contains("error")) {
            response.success = false;
            const auto& error = json_response["error"];
            
            if (error.contains("message")) {
                response.error_message = error["message"].get<std::string>();
            } else {
                response.error_message = "Unknown API error";
            }
            
            if (error.contains("type")) {
                response.metadata["error_type"] = error["type"].get<std::string>();
            }
            
            return response;
        }
        
        // Parse successful response
        response.success = true;
        response.model_used = json_response.value("model", "unknown");
        
        // Extract content from content array
        if (json_response.contains("content") && json_response["content"].is_array()) {
            const auto& content_array = json_response["content"];
            std::ostringstream content_stream;
            
            for (const auto& content_item : content_array) {
                if (content_item.contains("text")) {
                    content_stream << content_item["text"].get<std::string>();
                }
            }
            
            response.content = content_stream.str();
        }
        
        // Extract usage information
        if (json_response.contains("usage")) {
            const auto& usage = json_response["usage"];
            response.prompt_tokens = usage.value("input_tokens", 0);
            response.completion_tokens = usage.value("output_tokens", 0);
            response.tokens_used = response.prompt_tokens + response.completion_tokens;
        }
        
        // Extract additional metadata
        response.metadata["id"] = json_response.value("id", "");
        response.metadata["stop_reason"] = json_response.value("stop_reason", "");
        response.metadata["stop_sequence"] = json_response.value("stop_sequence", "");
        
    } catch (const nlohmann::json::exception& e) {
        response.success = false;
        response.error_message = "Response parsing error: " + std::string(e.what());
        
        Logger::getInstance().log(LogLevel::ERROR, response.error_message.value());
    }
    
    return response;
}

std::map<std::string, std::string> AnthropicProvider::create_headers() const {
    std::map<std::string, std::string> headers;
    
    headers["Content-Type"] = "application/json";
    headers["x-api-key"] = m_config.get_api_key("anthropic");
    headers["anthropic-version"] = API_VERSION;
    headers["User-Agent"] = "CQL-AnthropicProvider/1.0";
    
    return headers;
}

std::optional<StreamingChunk> AnthropicProvider::parse_stream_chunk(const std::string& chunk) const {
    // Anthropic SSE format: "data: {json}\n\n"
    if (!chunk.starts_with("data: ")) {
        return std::nullopt;
    }
    
    try {
        std::string json_str = chunk.substr(6); // Remove "data: " prefix
        
        // Remove trailing newlines
        while (!json_str.empty() && (json_str.back() == '\n' || json_str.back() == '\r')) {
            json_str.pop_back();
        }
        
        // Handle special SSE events
        if (json_str == "[DONE]") {
            StreamingChunk final_chunk;
            final_chunk.is_final = true;
            return final_chunk;
        }
        
        nlohmann::json json_chunk = nlohmann::json::parse(json_str);
        StreamingChunk streaming_chunk;
        
        // Check for errors
        if (json_chunk.contains("error")) {
            streaming_chunk.error = json_chunk["error"]["message"].get<std::string>();
            streaming_chunk.is_final = true;
            return streaming_chunk;
        }
        
        // Extract content delta
        if (json_chunk.contains("delta") && json_chunk["delta"].contains("text")) {
            streaming_chunk.content = json_chunk["delta"]["text"].get<std::string>();
        }
        
        // Check if this is the final chunk
        if (json_chunk.contains("type")) {
            std::string event_type = json_chunk["type"].get<std::string>();
            streaming_chunk.is_final = (event_type == "message_stop" || event_type == "content_block_stop");
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
