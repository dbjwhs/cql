// MIT License
// Copyright (c) 2025 dbjwhs

#include "mcp_tools.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/parser.hpp"
#include "../../include/cql/compiler.hpp"
#include "../../include/cql/lexer.hpp"
#include <sstream>

namespace cql::mcp {

nlohmann::json get_tools_list() {
    nlohmann::json tools = nlohmann::json::array();

    // compile_prompt tool
    {
        nlohmann::json tool;
        tool["name"] = "compile_prompt";
        tool["description"] = "Compile a .llm file content into a formatted prompt";
        tool["inputSchema"] = {
            {"type", "object"},
            {"properties", {
                {"content", {{"type", "string"}, {"description", "The .llm file content to compile"}}}
            }},
            {"required", nlohmann::json::array({"content"})}
        };
        tools.push_back(tool);
    }

    // validate_llm_file tool
    {
        nlohmann::json tool;
        tool["name"] = "validate_llm_file";
        tool["description"] = "Validate .llm file syntax and report all errors";
        tool["inputSchema"] = {
            {"type", "object"},
            {"properties", {
                {"content", {{"type", "string"}, {"description", "The .llm file content to validate"}}}
            }},
            {"required", nlohmann::json::array({"content"})}
        };
        tools.push_back(tool);
    }

    // list_directives tool
    {
        nlohmann::json tool;
        tool["name"] = "list_directives";
        tool["description"] = "List all available CQL directives with descriptions";
        tool["inputSchema"] = {
            {"type", "object"},
            {"properties", nlohmann::json::object()}
        };
        tools.push_back(tool);
    }

    return tools;
}

nlohmann::json handle_tool_call(const std::string& tool_name,
                                const nlohmann::json& arguments) {
    nlohmann::json result;

    if (tool_name == "compile_prompt") {
        std::string content = arguments.value("content", "");
        if (content.empty()) {
            result["content"] = nlohmann::json::array({
                {{"type", "text"}, {"text", "Error: 'content' parameter is required"}}
            });
            result["isError"] = true;
            return result;
        }

        try {
            std::string compiled = cql::QueryProcessor::compile(content);
            result["content"] = nlohmann::json::array({
                {{"type", "text"}, {"text", compiled}}
            });
        } catch (const std::exception& e) {
            result["content"] = nlohmann::json::array({
                {{"type", "text"}, {"text", std::string("Compilation error: ") + e.what()}}
            });
            result["isError"] = true;
        }

    } else if (tool_name == "validate_llm_file") {
        std::string content = arguments.value("content", "");
        if (content.empty()) {
            result["content"] = nlohmann::json::array({
                {{"type", "text"}, {"text", "Error: 'content' parameter is required"}}
            });
            result["isError"] = true;
            return result;
        }

        try {
            cql::Parser parser(content);
            parser.parse();
            result["content"] = nlohmann::json::array({
                {{"type", "text"}, {"text", "Valid: no errors found"}}
            });
        } catch (const cql::ParserError& e) {
            result["content"] = nlohmann::json::array({
                {{"type", "text"}, {"text", std::string(e.what())}}
            });
            result["isError"] = true;
        } catch (const cql::LexerError& e) {
            result["content"] = nlohmann::json::array({
                {{"type", "text"}, {"text", std::string(e.what())}}
            });
            result["isError"] = true;
        }

    } else if (tool_name == "list_directives") {
        std::ostringstream oss;
        oss << "Available CQL directives:\n\n";
        oss << "@language    - Target programming language\n";
        oss << "@description - Description of code to generate\n";
        oss << "@context     - Additional context information\n";
        oss << "@test        - Test case requirements\n";
        oss << "@dependency  - External dependencies\n";
        oss << "@performance - Performance requirements\n";
        oss << "@copyright   - Copyright and license info\n";
        oss << "@architecture - Architecture patterns\n";
        oss << "@constraint  - Code constraints\n";
        oss << "@example     - Code examples with labels\n";
        oss << "@security    - Security requirements\n";
        oss << "@complexity  - Algorithm complexity requirements\n";
        oss << "@model       - Target LLM model\n";
        oss << "@format      - Output format (markdown, json)\n";
        oss << "@variable    - Template variable definition\n";
        oss << "@output_format - File output format\n";
        oss << "@max_tokens  - Maximum token limit\n";
        oss << "@temperature - Generation temperature\n";
        oss << "@pattern     - Design pattern description\n";
        oss << "@structure   - File structure definition\n";
        oss << "@provider    - AI provider selection (anthropic, openai)\n";

        result["content"] = nlohmann::json::array({
            {{"type", "text"}, {"text", oss.str()}}
        });

    } else {
        result["content"] = nlohmann::json::array({
            {{"type", "text"}, {"text", "Unknown tool: " + tool_name}}
        });
        result["isError"] = true;
    }

    return result;
}

} // namespace cql::mcp
