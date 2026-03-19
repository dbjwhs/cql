// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_MCP_TOOLS_HPP
#define CQL_MCP_TOOLS_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace cql::mcp {

/**
 * @brief Get the list of all available MCP tools
 */
[[nodiscard]] nlohmann::json get_tools_list();

/**
 * @brief Handle a tools/call request
 * @param tool_name Name of the tool to invoke
 * @param arguments Tool arguments
 * @return Tool result as JSON
 */
[[nodiscard]] nlohmann::json handle_tool_call(const std::string& tool_name,
                                              const nlohmann::json& arguments);

} // namespace cql::mcp

#endif // CQL_MCP_TOOLS_HPP
