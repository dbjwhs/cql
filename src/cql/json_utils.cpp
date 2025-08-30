// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/json_utils.hpp"
#include <stdexcept>

namespace cql {

// ============================================================================
// API Request Creation
// ============================================================================

nlohmann::json JsonUtils::create_api_request(
    const std::string& model,
    const std::string& query,
    int max_tokens,
    double temperature,
    bool streaming) {
    
    nlohmann::json request = nlohmann::json::object();
    
    // Core API fields
    request["model"] = model;
    request["max_tokens"] = max_tokens;
    request["temperature"] = temperature;
    request["messages"] = create_messages_array(query);
    
    // Optional streaming
    if (streaming) {
        request["stream"] = true;
    }
    
    return request;
}

nlohmann::json JsonUtils::create_messages_array(
    const std::string& content,
    const std::string& role) {
    
    nlohmann::json messages = nlohmann::json::array();
    
    nlohmann::json message = nlohmann::json::object();
    message["role"] = role;
    message["content"] = content;
    
    messages.push_back(message);
    
    return messages;
}

// ============================================================================
// Response Creation (for testing/mocking)
// ============================================================================

nlohmann::json JsonUtils::create_mock_response(
    const std::string& content,
    const std::string& model,
    const std::string& message_id) {
    
    nlohmann::json response = nlohmann::json::object();
    
    // Response metadata
    response["id"] = message_id;
    response["type"] = "message";
    response["role"] = "assistant";
    response["model"] = model;
    
    // Content array
    nlohmann::json content_array = nlohmann::json::array();
    nlohmann::json content_item = nlohmann::json::object();
    content_item["type"] = "text";
    content_item["text"] = content;
    content_array.push_back(content_item);
    response["content"] = content_array;
    
    // Response completion info
    response["stop_reason"] = "end_turn";
    response["stop_sequence"] = nullptr;
    
    // Usage statistics
    response["usage"] = create_usage_stats(100, 500);
    
    return response;
}

nlohmann::json JsonUtils::create_error_response(
    int status_code,
    const std::string& error_type,
    const std::string& error_message) {
    
    nlohmann::json response = nlohmann::json::object();
    
    nlohmann::json error = nlohmann::json::object();
    error["type"] = error_type;
    error["message"] = error_message;
    error["status"] = status_code;
    
    response["error"] = error;
    
    return response;
}

nlohmann::json JsonUtils::create_usage_stats(
    int input_tokens,
    int output_tokens) {
    
    nlohmann::json usage = nlohmann::json::object();
    usage["input_tokens"] = input_tokens;
    usage["output_tokens"] = output_tokens;
    
    return usage;
}

// ============================================================================
// JSON Parsing and Validation
// ============================================================================

std::optional<nlohmann::json> JsonUtils::safe_parse(const std::string& json_str) {
    try {
        return nlohmann::json::parse(json_str);
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

std::string JsonUtils::get_string(
    const nlohmann::json& json, 
    const std::string& field, 
    const std::string& default_value) {
    
    if (json.contains(field) && json[field].is_string()) {
        return json[field].get<std::string>();
    }
    return default_value;
}

int JsonUtils::get_int(
    const nlohmann::json& json, 
    const std::string& field, 
    int default_value) {
    
    if (json.contains(field) && json[field].is_number_integer()) {
        return json[field].get<int>();
    }
    return default_value;
}

double JsonUtils::get_double(
    const nlohmann::json& json, 
    const std::string& field, 
    double default_value) {
    
    if (json.contains(field) && json[field].is_number()) {
        return json[field].get<double>();
    }
    return default_value;
}

bool JsonUtils::get_bool(
    const nlohmann::json& json, 
    const std::string& field, 
    bool default_value) {
    
    if (json.contains(field) && json[field].is_boolean()) {
        return json[field].get<bool>();
    }
    return default_value;
}

// ============================================================================
// Config JSON Utilities
// ============================================================================

bool JsonUtils::parse_config(
    const nlohmann::json& config_json,
    std::string& api_key,
    std::string& model,
    std::string& base_url,
    int& max_tokens,
    double& temperature) {
    
    try {
        if (config_json.contains("api") && config_json["api"].is_object()) {
            const auto& api = config_json["api"];
            
            api_key = get_string(api, "key", "");
            model = get_string(api, "model", "claude-3-opus");
            base_url = get_string(api, "base_url", "https://api.anthropic.com");
            max_tokens = get_int(api, "max_tokens", 1024);
            temperature = get_double(api, "temperature", 0.7);
            
            return true;
        }
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

// ============================================================================
// Formatting Utilities
// ============================================================================

std::string JsonUtils::to_pretty_string(const nlohmann::json& json, int indent) {
    return json.dump(indent);
}

std::string JsonUtils::to_compact_string(const nlohmann::json& json) {
    return json.dump();
}

} // namespace cql
