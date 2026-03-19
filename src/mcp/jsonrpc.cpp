// MIT License
// Copyright (c) 2025 dbjwhs

#include "jsonrpc.hpp"
#include <sstream>

namespace cql::mcp {

std::optional<JsonRpcRequest> parse_request(const std::string& json_str) {
    try {
        auto json = nlohmann::json::parse(json_str);

        if (!json.contains("jsonrpc") || json["jsonrpc"] != "2.0") {
            return std::nullopt;
        }
        if (!json.contains("method") || !json["method"].is_string()) {
            return std::nullopt;
        }

        JsonRpcRequest req;
        req.jsonrpc = json["jsonrpc"].get<std::string>();
        req.method = json["method"].get<std::string>();

        if (json.contains("params")) {
            req.params = json["params"];
        }

        if (json.contains("id")) {
            req.id = json["id"];
        } else {
            req.id = nullptr;
        }

        return req;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

nlohmann::json make_response(const nlohmann::json& id, const nlohmann::json& result) {
    nlohmann::json resp;
    resp["jsonrpc"] = "2.0";
    resp["result"] = result;
    resp["id"] = id;
    return resp;
}

nlohmann::json make_error(const nlohmann::json& id, int code, const std::string& message) {
    nlohmann::json resp;
    resp["jsonrpc"] = "2.0";
    resp["error"] = {{"code", code}, {"message", message}};
    resp["id"] = id;
    return resp;
}

std::string serialize_response(const nlohmann::json& response) {
    return response.dump();
}

} // namespace cql::mcp
