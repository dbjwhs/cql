// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_JSON_UTILS_HPP
#define CQL_JSON_UTILS_HPP

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include "../../third_party/include/nlohmann/json.hpp"

namespace cql {

/**
 * @class JsonUtils
 * @brief Unified JSON creation and parsing utilities
 * 
 * Centralizes JSON operations to eliminate code duplication and
 * provide consistent JSON handling throughout the application.
 */
class JsonUtils {
public:
    // ============================================================================
    // API Request Creation
    // ============================================================================
    
    /**
     * @brief Create a Claude API request JSON
     * @param model Model name to use
     * @param query User query/prompt
     * @param max_tokens Maximum tokens in response
     * @param temperature Temperature setting (0.0-1.0)
     * @param streaming Enable streaming response
     * @return Complete API request JSON object
     */
    static nlohmann::json create_api_request(
        const std::string& model,
        const std::string& query,
        int max_tokens = 1024,
        double temperature = 0.7,
        bool streaming = false
    );
    
    /**
     * @brief Create a messages array for Claude API
     * @param content User message content
     * @param role Message role (default: "user")
     * @return Messages array suitable for Claude API
     */
    static nlohmann::json create_messages_array(
        const std::string& content,
        const std::string& role = "user"
    );
    
    // ============================================================================
    // Response Creation (for testing/mocking)
    // ============================================================================
    
    /**
     * @brief Create a mock Claude API response
     * @param content Response text content
     * @param model Model name (optional)
     * @param message_id Message ID (optional)
     * @return Complete mock API response JSON
     */
    static nlohmann::json create_mock_response(
        const std::string& content,
        const std::string& model = "claude-3-opus-20240229",
        const std::string& message_id = "msg_mock123456789"
    );
    
    /**
     * @brief Create a mock error response
     * @param status_code HTTP status code
     * @param error_type Error type string
     * @param error_message Error message
     * @return Complete error response JSON
     */
    static nlohmann::json create_error_response(
        int status_code,
        const std::string& error_type,
        const std::string& error_message
    );
    
    /**
     * @brief Create usage statistics object
     * @param input_tokens Input token count
     * @param output_tokens Output token count
     * @return Usage statistics JSON object
     */
    static nlohmann::json create_usage_stats(
        int input_tokens,
        int output_tokens
    );
    
    // ============================================================================
    // JSON Parsing and Validation
    // ============================================================================
    
    /**
     * @brief Safe JSON parsing with error handling
     * @param json_str JSON string to parse
     * @return Parsed JSON object or std::nullopt on error
     */
    static std::optional<nlohmann::json> safe_parse(const std::string& json_str);
    
    /**
     * @brief Extract string field from JSON with default
     * @param json JSON object
     * @param field Field name
     * @param default_value Default value if field missing
     * @return Field value or default
     */
    static std::string get_string(
        const nlohmann::json& json, 
        const std::string& field, 
        const std::string& default_value = ""
    );
    
    /**
     * @brief Extract integer field from JSON with default
     * @param json JSON object
     * @param field Field name
     * @param default_value Default value if field missing
     * @return Field value or default
     */
    static int get_int(
        const nlohmann::json& json, 
        const std::string& field, 
        int default_value = 0
    );
    
    /**
     * @brief Extract double field from JSON with default
     * @param json JSON object
     * @param field Field name
     * @param default_value Default value if field missing
     * @return Field value or default
     */
    static double get_double(
        const nlohmann::json& json, 
        const std::string& field, 
        double default_value = 0.0
    );
    
    /**
     * @brief Extract boolean field from JSON with default
     * @param json JSON object
     * @param field Field name
     * @param default_value Default value if field missing
     * @return Field value or default
     */
    static bool get_bool(
        const nlohmann::json& json, 
        const std::string& field, 
        bool default_value = false
    );
    
    // ============================================================================
    // Config JSON Utilities
    // ============================================================================
    
    /**
     * @brief Parse configuration JSON safely
     * @param config_json JSON object containing configuration
     * @param api_key Output parameter for API key
     * @param model Output parameter for model name
     * @param base_url Output parameter for base URL
     * @param max_tokens Output parameter for max tokens
     * @param temperature Output parameter for temperature
     * @return true if parsing succeeded, false otherwise
     */
    static bool parse_config(
        const nlohmann::json& config_json,
        std::string& api_key,
        std::string& model,
        std::string& base_url,
        int& max_tokens,
        double& temperature
    );
    
    // ============================================================================
    // Formatting Utilities
    // ============================================================================
    
    /**
     * @brief Convert JSON to pretty-printed string
     * @param json JSON object to format
     * @param indent Number of spaces for indentation (default: 2)
     * @return Formatted JSON string
     */
    static std::string to_pretty_string(const nlohmann::json& json, int indent = 2);
    
    /**
     * @brief Convert JSON to compact string (no formatting)
     * @param json JSON object to convert
     * @return Compact JSON string
     */
    static std::string to_compact_string(const nlohmann::json& json);
};

} // namespace cql

#endif // CQL_JSON_UTILS_HPP