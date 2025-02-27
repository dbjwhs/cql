// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/parser.hpp"

namespace cql {

// ParserError implementation
ParserError::ParserError(const std::string& message, size_t line, size_t column)
    : std::runtime_error("Parser error at line " + std::to_string(line) + 
                         ", column " + std::to_string(column) + ": " + message),
      m_line(line), m_column(column) {}

// Parser implementation
Parser::Parser(const std::string_view input) : m_lexer(input) {
    advance();
}

void Parser::advance() {
    m_current_token = m_lexer.next_token();
}

std::vector<std::unique_ptr<QueryNode>> Parser::parse() {
    std::vector<std::unique_ptr<QueryNode>> nodes;

    while (m_current_token) {
        if (m_current_token->m_type == TokenType::NEWLINE) {
            advance();
            continue;
        }

        // Validate the token is a keyword
        if (m_current_token->m_type != TokenType::LANGUAGE &&
            m_current_token->m_type != TokenType::DESCRIPTION &&
            m_current_token->m_type != TokenType::CONTEXT &&
            m_current_token->m_type != TokenType::TEST &&
            m_current_token->m_type != TokenType::DEPENDENCY &&
            m_current_token->m_type != TokenType::PERFORMANCE &&
            m_current_token->m_type != TokenType::COPYRIGHT &&
            // Phase 2 directives
            m_current_token->m_type != TokenType::ARCHITECTURE &&
            m_current_token->m_type != TokenType::CONSTRAINT &&
            m_current_token->m_type != TokenType::EXAMPLE &&
            m_current_token->m_type != TokenType::SECURITY &&
            m_current_token->m_type != TokenType::COMPLEXITY &&
            m_current_token->m_type != TokenType::MODEL &&
            m_current_token->m_type != TokenType::FORMAT &&
            m_current_token->m_type != TokenType::VARIABLE) {
            
            if (!m_current_token) {
                throw ParserError("Expected keyword but reached end of input", 
                                  m_lexer.current_line(), m_lexer.current_column());
            } else {
                throw ParserError("Expected keyword", 
                                  m_current_token->m_line, m_current_token->m_column);
            }
        }

        // Parse the appropriate node type
        switch (m_current_token->m_type) {
            case TokenType::LANGUAGE:
                nodes.push_back(parse_code_request());
                break;
            case TokenType::CONTEXT:
                nodes.push_back(parse_context());
                break;
            case TokenType::TEST:
                nodes.push_back(parse_test());
                break;
            case TokenType::DEPENDENCY:
                nodes.push_back(parse_dependency());
                break;
            case TokenType::PERFORMANCE:
                nodes.push_back(parse_performance());
                break;
            case TokenType::COPYRIGHT:
                nodes.push_back(parse_copyright());
                break;
            // Phase 2 directives
            case TokenType::ARCHITECTURE:
                nodes.push_back(parse_architecture());
                break;
            case TokenType::CONSTRAINT:
                nodes.push_back(parse_constraint());
                break;
            case TokenType::EXAMPLE:
                nodes.push_back(parse_example());
                break;
            case TokenType::SECURITY:
                nodes.push_back(parse_security());
                break;
            case TokenType::COMPLEXITY:
                nodes.push_back(parse_complexity());
                break;
            case TokenType::MODEL:
                nodes.push_back(parse_model());
                break;
            case TokenType::FORMAT:
                nodes.push_back(parse_format());
                break;
            case TokenType::VARIABLE:
                nodes.push_back(parse_variable());
                break;
            default:
                throw ParserError("Unexpected token type", 
                                 m_current_token->m_line, m_current_token->m_column);
        }
    }

    return nodes;
}

std::string Parser::parse_string() {
    // Skip whitespace tokens
    while (m_current_token && m_current_token->m_type == TokenType::NEWLINE) {
        advance();
    }

    if (!m_current_token) {
        throw ParserError("Unexpected end of input while expecting string", 
                         m_lexer.current_line(), m_lexer.current_column());
    }

    if (m_current_token->m_type != TokenType::STRING) {
        throw ParserError(
            "Expected string (got " + token_type_to_string(m_current_token->m_type) +
            " with value '" + m_current_token->m_value + "')",
            m_current_token->m_line, m_current_token->m_column
        );
    }

    std::string value = m_current_token->m_value;
    advance(); // Move to the next token
    return value;
}

std::unique_ptr<QueryNode> Parser::parse_code_request() {
    size_t line = m_current_token->m_line;
    size_t column = m_current_token->m_column;
    
    advance(); // Skip past @language token

    // Get the language string
    std::string language = parse_string();

    // Skip any newlines
    while (m_current_token && m_current_token->m_type == TokenType::NEWLINE) {
        advance();
    }

    // Now expect @description
    if (!m_current_token || m_current_token->m_type != TokenType::DESCRIPTION) {
        throw ParserError(
            "Expected @description after @language",
            m_current_token ? m_current_token->m_line : line,
            m_current_token ? m_current_token->m_column : column
        );
    }

    advance(); // Skip past @description token
    std::string description = parse_string();

    return std::make_unique<CodeRequestNode>(language, description);
}

std::unique_ptr<QueryNode> Parser::parse_context() {
    advance(); // Skip @context
    std::string context = parse_string();
    return std::make_unique<ContextNode>(context);
}

std::unique_ptr<QueryNode> Parser::parse_test() {
    advance(); // Skip @test
    std::vector<std::string> test_cases;

    // Get the test case
    test_cases.push_back(parse_string());

    return std::make_unique<TestNode>(std::move(test_cases));
}

std::unique_ptr<QueryNode> Parser::parse_dependency() {
    advance(); // Skip @dependency
    std::vector<std::string> dependencies;

    // Get the dependency
    dependencies.push_back(parse_string());

    return std::make_unique<DependencyNode>(std::move(dependencies));
}

std::unique_ptr<QueryNode> Parser::parse_performance() {
    advance(); // Skip @performance
    std::string requirement = parse_string();
    return std::make_unique<PerformanceNode>(requirement);
}

std::unique_ptr<QueryNode> Parser::parse_copyright() {
    advance(); // Skip @copyright
    std::string license = parse_string();
    std::string owner = parse_string();
    return std::make_unique<CopyrightNode>(license, owner);
}

std::unique_ptr<QueryNode> Parser::parse_architecture() {
    advance(); // Skip @architecture
    std::string architecture = parse_string();
    return std::make_unique<ArchitectureNode>(architecture);
}

std::unique_ptr<QueryNode> Parser::parse_constraint() {
    advance(); // Skip @constraint
    std::string constraint = parse_string();
    return std::make_unique<ConstraintNode>(constraint);
}

std::unique_ptr<QueryNode> Parser::parse_example() {
    advance(); // Skip @example
    std::string label = parse_string();
    std::string code = parse_string();
    return std::make_unique<ExampleNode>(label, code);
}

std::unique_ptr<QueryNode> Parser::parse_security() {
    advance(); // Skip @security
    std::string requirement = parse_string();
    return std::make_unique<SecurityNode>(requirement);
}

std::unique_ptr<QueryNode> Parser::parse_complexity() {
    advance(); // Skip @complexity
    std::string complexity = parse_string();
    return std::make_unique<ComplexityNode>(complexity);
}

std::unique_ptr<QueryNode> Parser::parse_model() {
    advance(); // Skip @model
    std::string model_name = parse_string();
    return std::make_unique<ModelNode>(model_name);
}

std::unique_ptr<QueryNode> Parser::parse_format() {
    advance(); // Skip @format
    std::string format_type = parse_string();
    return std::make_unique<FormatNode>(format_type);
}

std::unique_ptr<QueryNode> Parser::parse_variable() {
    advance(); // Skip @variable
    std::string name = parse_string();
    std::string value = parse_string();
    return std::make_unique<VariableNode>(name, value);
}

} // namespace cql