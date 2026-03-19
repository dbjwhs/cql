// MIT License
// Copyright (c) 2025 dbjwhs

#include "ailib/providers/openai.hpp"
#include "ailib/detail/json_utils.hpp"
#include "../../../../include/cql/project_utils.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <future>

namespace cql {

namespace {

// Supported OpenAI models and their specifications
const std::map<std::string, std::pair<size_t, size_t>> OPENAI_MODELS = {
    {"gpt-4o", {128000, 16384}},
    {"gpt-4o-2024-11-20", {128000, 16384}},
    {"gpt-4o-mini", {128000, 16384}},
    {"gpt-4o-mini-2024-07-18", {128000, 16384}},
    {"gpt-4-turbo", {128000, 4096}},
    {"gpt-4-turbo-2024-04-09", {128000, 4096}},
    {"gpt-4", {8192, 4096}},
    {"gpt-4-0613", {8192, 4096}},
    {"gpt-3.5-turbo", {16385, 4096}},
    {"o1", {200000, 100000}},
    {"o1-mini", {128000, 65536}},
    {"o3-mini", {200000, 100000}}
};

// Token pricing per model (input, output) per 1K tokens in USD
const std::map<std::string, std::pair<double, double>> OPENAI_PRICING = {
    {"gpt-4o", {0.0025, 0.01}},
    {"gpt-4o-2024-11-20", {0.0025, 0.01}},
    {"gpt-4o-mini", {0.00015, 0.0006}},
    {"gpt-4o-mini-2024-07-18", {0.00015, 0.0006}},
    {"gpt-4-turbo", {0.01, 0.03}},
    {"gpt-4-turbo-2024-04-09", {0.01, 0.03}},
    {"gpt-4", {0.03, 0.06}},
    {"gpt-4-0613", {0.03, 0.06}},
    {"gpt-3.5-turbo", {0.0005, 0.0015}},
    {"o1", {0.015, 0.06}},
    {"o1-mini", {0.003, 0.012}},
    {"o3-mini", {0.0011, 0.0044}}
};

} // anonymous namespace

OpenAIProvider::OpenAIProvider(const Config& config)
    : m_config(config) {

    Logger::getInstance().log(LogLevel::DEBUG, "OpenAIProvider constructor called");

    http::ClientConfig http_config;
    http_config.default_timeout = std::chrono::seconds(120);
    http_config.verify_ssl = true;
    http_config.enable_compression = true;

    auto headers = create_headers();
    for (const auto& [key, value] : headers) {
        http_config.default_headers[key] = value;
    }

    m_http_client = http::ClientFactory::create_default(http_config);

    Logger::getInstance().log(LogLevel::INFO,
        "OpenAIProvider initialized with base URL: ",
        m_config.get_base_url("openai").value_or(BASE_URL));
}

ProviderResponse OpenAIProvider::generate(const ProviderRequest& request) {
    Logger::getInstance().log(LogLevel::INFO, "Generating response with model: ", request.model);

    auto start_time = std::chrono::steady_clock::now();

    try {
        nlohmann::json api_request = convert_request(request);
        std::string request_body = api_request.dump();

        std::string base_url = m_config.get_base_url("openai").value_or(BASE_URL);
        http::Request http_request;
        http_request.url = base_url + "/v1/chat/completions";
        http_request.method = "POST";
        http_request.body = request_body;
        http_request.headers = create_headers();
        http_request.timeout = std::chrono::seconds(120);
        http_request.retry_policy.max_retries = m_config.get_max_retries("openai");

        http::Response response = m_http_client->send(http_request);

        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

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

std::future<ProviderResponse> OpenAIProvider::generate_async(const ProviderRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return generate(request);
    });
}

void OpenAIProvider::generate_stream(const ProviderRequest& request, StreamingCallback callback) {
    Logger::getInstance().log(LogLevel::INFO, "Starting streaming response with model: ", request.model);

    try {
        nlohmann::json api_request = convert_request(request);
        api_request["stream"] = true;
        std::string request_body = api_request.dump();

        std::string base_url = m_config.get_base_url("openai").value_or(BASE_URL);
        http::Request http_request;
        http_request.url = base_url + "/v1/chat/completions";
        http_request.method = "POST";
        http_request.body = request_body;
        http_request.headers = create_headers();
        http_request.timeout = std::chrono::seconds(300);
        http_request.retry_policy.max_retries = std::min(1, m_config.get_max_retries("openai"));

        m_http_client->send_stream(http_request, [this, callback](const std::string& chunk) {
            auto streaming_chunk = parse_stream_chunk(chunk);
            if (streaming_chunk) {
                callback(streaming_chunk.value());
            }
        });

    } catch (const std::exception& e) {
        StreamingChunk error_chunk;
        error_chunk.error = "Streaming error: " + std::string(e.what());
        error_chunk.is_final = true;
        callback(error_chunk);
        Logger::getInstance().log(LogLevel::ERROR, error_chunk.error.value());
    }
}

std::string OpenAIProvider::get_provider_name() const {
    return "OpenAI";
}

ProviderCapabilities OpenAIProvider::get_capabilities() const {
    ProviderCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_functions = true;
    caps.supports_vision = true;
    caps.supports_async = true;

    for (const auto& [model, _] : OPENAI_MODELS) {
        caps.available_models.push_back(model);
    }

    caps.max_context_length = 128000;
    caps.max_output_tokens = 16384;

    return caps;
}

bool OpenAIProvider::is_configured() const {
    std::string api_key = m_config.get_api_key("openai");
    return !api_key.empty() && api_key.length() >= 20;
}

bool OpenAIProvider::validate_model(const std::string& model) const {
    return OPENAI_MODELS.find(model) != OPENAI_MODELS.end();
}

std::optional<double> OpenAIProvider::estimate_cost(const ProviderRequest& request) const {
    auto pricing_it = OPENAI_PRICING.find(request.model);
    if (pricing_it == OPENAI_PRICING.end()) {
        return std::nullopt;
    }

    const auto& [input_price, output_price] = pricing_it->second;

    double estimated_input_tokens = request.prompt.length() / 4.0;
    double estimated_output_tokens = request.max_tokens;

    if (request.system_prompt) {
        estimated_input_tokens += request.system_prompt->length() / 4.0;
    }

    for (const auto& [role, content] : request.messages) {
        estimated_input_tokens += content.length() / 4.0;
    }

    double input_cost = (estimated_input_tokens / 1000.0) * input_price;
    double output_cost = (estimated_output_tokens / 1000.0) * output_price;
    return input_cost + output_cost;
}

nlohmann::json OpenAIProvider::convert_request(const ProviderRequest& request) const {
    nlohmann::json api_request = nlohmann::json::object();

    api_request["model"] = request.model;
    api_request["max_tokens"] = request.max_tokens;

    // Messages array — system prompt goes as a message, not a top-level field
    nlohmann::json messages = nlohmann::json::array();

    // Add system prompt as first message
    if (request.system_prompt) {
        nlohmann::json sys_msg;
        sys_msg["role"] = "system";
        sys_msg["content"] = request.system_prompt.value();
        messages.push_back(sys_msg);
    }

    if (!request.messages.empty()) {
        for (const auto& [role, content] : request.messages) {
            nlohmann::json message;
            message["role"] = role;
            message["content"] = content;
            messages.push_back(message);
        }

        if (request.messages.back().first != "user") {
            nlohmann::json user_message;
            user_message["role"] = "user";
            user_message["content"] = request.prompt;
            messages.push_back(user_message);
        }
    } else {
        nlohmann::json user_message;
        user_message["role"] = "user";
        user_message["content"] = request.prompt;
        messages.push_back(user_message);
    }

    api_request["messages"] = messages;

    if (request.temperature != 0.7) {
        api_request["temperature"] = request.temperature;
    }

    if (request.top_p) {
        api_request["top_p"] = request.top_p.value();
    }

    // Add provider-specific metadata
    for (const auto& [key, value] : request.metadata) {
        if (key.starts_with("openai_")) {
            std::string api_key = key.substr(7); // Remove "openai_" prefix
            api_request[api_key] = value;
        }
    }

    return api_request;
}

ProviderResponse OpenAIProvider::parse_response(const nlohmann::json& json_response,
                                                std::chrono::milliseconds latency) const {
    ProviderResponse response;
    response.latency = latency;

    try {
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

        response.success = true;
        response.model_used = json_response.value("model", "unknown");

        // OpenAI response: choices[0].message.content
        if (json_response.contains("choices") && json_response["choices"].is_array() &&
            !json_response["choices"].empty()) {
            const auto& choice = json_response["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                response.content = choice["message"]["content"].get<std::string>();
            }

            if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
                response.metadata["stop_reason"] = choice["finish_reason"].get<std::string>();
            }
        }

        // OpenAI usage: usage.prompt_tokens / usage.completion_tokens
        if (json_response.contains("usage")) {
            const auto& usage = json_response["usage"];
            response.prompt_tokens = usage.value("prompt_tokens", 0);
            response.completion_tokens = usage.value("completion_tokens", 0);
            response.tokens_used = response.prompt_tokens + response.completion_tokens;
        }

        response.metadata["id"] = json_response.value("id", "");

    } catch (const nlohmann::json::exception& e) {
        response.success = false;
        response.error_message = "Response parsing error: " + std::string(e.what());
        Logger::getInstance().log(LogLevel::ERROR, response.error_message.value());
    }

    return response;
}

std::map<std::string, std::string> OpenAIProvider::create_headers() const {
    std::map<std::string, std::string> headers;

    headers["Content-Type"] = "application/json";
    // OpenAI uses Bearer token auth, not x-api-key
    headers["Authorization"] = "Bearer " + m_config.get_api_key("openai");
    headers["User-Agent"] = "CQL-OpenAIProvider/1.0";

    return headers;
}

std::optional<StreamingChunk> OpenAIProvider::parse_stream_chunk(const std::string& chunk) const {
    // OpenAI SSE format: "data: {json}\n\n"
    if (!chunk.starts_with("data: ")) {
        return std::nullopt;
    }

    try {
        std::string json_str = chunk.substr(6);

        while (!json_str.empty() && (json_str.back() == '\n' || json_str.back() == '\r')) {
            json_str.pop_back();
        }

        // OpenAI sentinel for end of stream
        if (json_str == "[DONE]") {
            StreamingChunk final_chunk;
            final_chunk.is_final = true;
            return final_chunk;
        }

        nlohmann::json json_chunk = nlohmann::json::parse(json_str);
        StreamingChunk streaming_chunk;

        if (json_chunk.contains("error")) {
            streaming_chunk.error = json_chunk["error"]["message"].get<std::string>();
            streaming_chunk.is_final = true;
            return streaming_chunk;
        }

        // OpenAI streaming: choices[0].delta.content
        if (json_chunk.contains("choices") && !json_chunk["choices"].empty()) {
            const auto& choice = json_chunk["choices"][0];
            if (choice.contains("delta") && choice["delta"].contains("content")) {
                streaming_chunk.content = choice["delta"]["content"].get<std::string>();
            }

            if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
                streaming_chunk.is_final = true;
            }
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
