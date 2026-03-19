// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_MCP_JSONRPC_HPP
#define CQL_MCP_JSONRPC_HPP

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace cql::mcp {

/**
 * @brief JSON-RPC 2.0 request structure
 */
struct JsonRpcRequest {
    std::string jsonrpc = "2.0";
    std::string method;
    std::optional<nlohmann::json> params;
    nlohmann::json id; // string, number, or null
};

/**
 * @brief JSON-RPC 2.0 response structure
 */
struct JsonRpcResponse {
    std::string jsonrpc = "2.0";
    std::optional<nlohmann::json> result;
    std::optional<nlohmann::json> error;
    nlohmann::json id;
};

/**
 * @brief JSON-RPC error codes
 */
namespace error_codes {
    constexpr int PARSE_ERROR = -32700;
    constexpr int INVALID_REQUEST = -32600;
    constexpr int METHOD_NOT_FOUND = -32601;
    constexpr int INVALID_PARAMS = -32602;
    constexpr int INTERNAL_ERROR = -32603;
}

/**
 * @brief Parse a JSON-RPC request from raw JSON
 */
[[nodiscard]] std::optional<JsonRpcRequest> parse_request(const std::string& json_str);

/**
 * @brief Create a success response
 */
[[nodiscard]] nlohmann::json make_response(const nlohmann::json& id, const nlohmann::json& result);

/**
 * @brief Create an error response
 */
[[nodiscard]] nlohmann::json make_error(const nlohmann::json& id, int code, const std::string& message);

/**
 * @brief Serialize a response to a JSON string
 */
[[nodiscard]] std::string serialize_response(const nlohmann::json& response);

} // namespace cql::mcp

#endif // CQL_MCP_JSONRPC_HPP
