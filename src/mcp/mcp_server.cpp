// MIT License
// Copyright (c) 2025 dbjwhs

#include "mcp_server.hpp"
#include "jsonrpc.hpp"
#include "mcp_tools.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

namespace cql::mcp {

int McpServer::run() {
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        std::string response = handle_request(line);
        if (!response.empty()) {
            std::cout << response << std::endl;
            std::cout.flush();
        }
    }

    return 0;
}

std::string McpServer::handle_request(const std::string& request_json) {
    auto req = parse_request(request_json);
    if (!req) {
        return serialize_response(
            make_error(nullptr, error_codes::PARSE_ERROR, "Invalid JSON-RPC request"));
    }

    nlohmann::json response;

    if (req->method == "initialize") {
        response = handle_initialize(req->id);
    } else if (req->method == "tools/list") {
        response = handle_tools_list(req->id);
    } else if (req->method == "tools/call") {
        response = handle_tools_call(req->id, req->params.value_or(nlohmann::json::object()));
    } else if (req->method == "shutdown") {
        response = make_response(req->id, nlohmann::json::object());
    } else if (req->method == "notifications/initialized") {
        // Notification — no response needed
        return "";
    } else {
        response = make_error(req->id, error_codes::METHOD_NOT_FOUND,
                             "Unknown method: " + req->method);
    }

    return serialize_response(response);
}

nlohmann::json McpServer::handle_initialize(const nlohmann::json& id) {
    m_initialized = true;

    nlohmann::json result;
    result["protocolVersion"] = "2024-11-05";
    result["capabilities"] = {
        {"tools", nlohmann::json::object()}
    };
    result["serverInfo"] = {
        {"name", "cql-mcp"},
        {"version", "1.0.0"}
    };

    return make_response(id, result);
}

nlohmann::json McpServer::handle_tools_list(const nlohmann::json& id) {
    nlohmann::json result;
    result["tools"] = get_tools_list();
    return make_response(id, result);
}

nlohmann::json McpServer::handle_tools_call(const nlohmann::json& id,
                                            const nlohmann::json& params) {
    if (!params.contains("name") || !params["name"].is_string()) {
        return make_error(id, error_codes::INVALID_PARAMS, "Missing 'name' parameter");
    }

    std::string tool_name = params["name"].get<std::string>();
    nlohmann::json arguments = params.value("arguments", nlohmann::json::object());

    nlohmann::json tool_result = handle_tool_call(tool_name, arguments);
    return make_response(id, tool_result);
}

} // namespace cql::mcp
