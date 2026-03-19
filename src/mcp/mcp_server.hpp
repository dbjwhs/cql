// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_MCP_SERVER_HPP
#define CQL_MCP_SERVER_HPP

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

namespace cql::mcp {

/**
 * @brief MCP (Model Context Protocol) server
 *
 * Implements the MCP specification using JSON-RPC over stdio.
 * Provides CQL tools to IDE integrations.
 */
class McpServer {
public:
    McpServer() = default;

    /**
     * @brief Run the MCP server event loop
     *
     * Reads JSON-RPC requests from stdin, dispatches them,
     * and writes responses to stdout. Runs until EOF or shutdown.
     *
     * @return Exit code (0 for clean shutdown)
     */
    int run();

private:
    /**
     * @brief Handle a single JSON-RPC request
     * @param request_json Raw JSON string
     * @return Response JSON string
     */
    [[nodiscard]] std::string handle_request(const std::string& request_json);

    /**
     * @brief Handle MCP initialize request
     */
    [[nodiscard]] nlohmann::json handle_initialize(const nlohmann::json& id);

    /**
     * @brief Handle tools/list request
     */
    [[nodiscard]] nlohmann::json handle_tools_list(const nlohmann::json& id);

    /**
     * @brief Handle tools/call request
     */
    [[nodiscard]] nlohmann::json handle_tools_call(const nlohmann::json& id,
                                                   const nlohmann::json& params);

    bool m_initialized = false;
};

} // namespace cql::mcp

#endif // CQL_MCP_SERVER_HPP
