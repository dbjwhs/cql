// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "../jsonrpc.hpp"
#include "../mcp_tools.hpp"
#include "../mcp_server.hpp"

namespace cql::mcp::test {

class McpJsonRpcTest : public ::testing::Test {};

TEST_F(McpJsonRpcTest, ParseValidRequest) {
    auto req = parse_request(R"({"jsonrpc":"2.0","method":"initialize","id":1})");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->method, "initialize");
    EXPECT_EQ(req->id, 1);
    EXPECT_FALSE(req->params.has_value());
}

TEST_F(McpJsonRpcTest, ParseRequestWithParams) {
    auto req = parse_request(R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"test"},"id":"abc"})");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->method, "tools/call");
    EXPECT_EQ(req->id, "abc");
    ASSERT_TRUE(req->params.has_value());
    EXPECT_EQ(req->params.value()["name"], "test");
}

TEST_F(McpJsonRpcTest, ParseInvalidRequest) {
    EXPECT_FALSE(parse_request("not json").has_value());
    EXPECT_FALSE(parse_request(R"({"method":"test"})").has_value()); // Missing jsonrpc
    EXPECT_FALSE(parse_request(R"({"jsonrpc":"1.0","method":"test"})").has_value()); // Wrong version
    EXPECT_FALSE(parse_request(R"({"jsonrpc":"2.0"})").has_value()); // Missing method
}

TEST_F(McpJsonRpcTest, MakeResponse) {
    auto resp = make_response(1, "hello");
    EXPECT_EQ(resp["jsonrpc"], "2.0");
    EXPECT_EQ(resp["result"], "hello");
    EXPECT_EQ(resp["id"], 1);
    EXPECT_FALSE(resp.contains("error"));
}

TEST_F(McpJsonRpcTest, MakeError) {
    auto resp = make_error(1, -32601, "Method not found");
    EXPECT_EQ(resp["jsonrpc"], "2.0");
    EXPECT_EQ(resp["error"]["code"], -32601);
    EXPECT_EQ(resp["error"]["message"], "Method not found");
    EXPECT_EQ(resp["id"], 1);
    EXPECT_FALSE(resp.contains("result"));
}

class McpToolsTest : public ::testing::Test {};

TEST_F(McpToolsTest, ToolsList) {
    auto tools = get_tools_list();
    ASSERT_TRUE(tools.is_array());
    EXPECT_GE(tools.size(), 3); // compile_prompt, validate_llm_file, list_directives

    // Check tool names
    bool has_compile = false;
    bool has_validate = false;
    bool has_list = false;
    for (const auto& tool : tools) {
        if (tool["name"] == "compile_prompt") has_compile = true;
        if (tool["name"] == "validate_llm_file") has_validate = true;
        if (tool["name"] == "list_directives") has_list = true;
    }
    EXPECT_TRUE(has_compile);
    EXPECT_TRUE(has_validate);
    EXPECT_TRUE(has_list);
}

TEST_F(McpToolsTest, CompilePromptValid) {
    nlohmann::json args = {{"content", "@copyright \"MIT\" \"test\"\n@language \"C++\"\n@description \"test program\"\n"}};
    auto result = handle_tool_call("compile_prompt", args);
    ASSERT_TRUE(result.contains("content"));
    EXPECT_FALSE(result.value("isError", false));
    auto text = result["content"][0]["text"].get<std::string>();
    EXPECT_TRUE(text.find("C++") != std::string::npos);
}

TEST_F(McpToolsTest, CompilePromptMissingContent) {
    nlohmann::json args = nlohmann::json::object();
    auto result = handle_tool_call("compile_prompt", args);
    EXPECT_TRUE(result.value("isError", false));
}

TEST_F(McpToolsTest, ValidateValid) {
    nlohmann::json args = {{"content", "@copyright \"MIT\" \"test\"\n@language \"C++\"\n@description \"test\"\n"}};
    auto result = handle_tool_call("validate_llm_file", args);
    EXPECT_FALSE(result.value("isError", false));
    auto text = result["content"][0]["text"].get<std::string>();
    EXPECT_TRUE(text.find("Valid") != std::string::npos || text.find("no errors") != std::string::npos);
}

TEST_F(McpToolsTest, ValidateInvalid) {
    nlohmann::json args = {{"content", "badtoken\n"}};
    auto result = handle_tool_call("validate_llm_file", args);
    EXPECT_TRUE(result.value("isError", false));
}

TEST_F(McpToolsTest, ListDirectives) {
    nlohmann::json args = nlohmann::json::object();
    auto result = handle_tool_call("list_directives", args);
    EXPECT_FALSE(result.value("isError", false));
    auto text = result["content"][0]["text"].get<std::string>();
    EXPECT_TRUE(text.find("@language") != std::string::npos);
    EXPECT_TRUE(text.find("@provider") != std::string::npos);
}

TEST_F(McpToolsTest, UnknownTool) {
    nlohmann::json args = nlohmann::json::object();
    auto result = handle_tool_call("nonexistent_tool", args);
    EXPECT_TRUE(result.value("isError", false));
}

} // namespace cql::mcp::test
