// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/parser.hpp"
#include <algorithm> // For std::transform
#include <utility>

namespace cql {

// Initialize the dispatch table for token type to parser function mapping
const std::unordered_map<TokenType, Parser::ParseFunction> Parser::m_dispatch_table = {
    {TokenType::LANGUAGE, &Parser::parse_code_request},
    {TokenType::CONTEXT, &Parser::parse_context},
    {TokenType::TEST, &Parser::parse_test},
    {TokenType::DEPENDENCY, &Parser::parse_dependency},
    {TokenType::PERFORMANCE, &Parser::parse_performance},
    {TokenType::COPYRIGHT, &Parser::parse_copyright},
    {TokenType::ARCHITECTURE, &Parser::parse_architecture},
    {TokenType::CONSTRAINT, &Parser::parse_constraint},
    {TokenType::EXAMPLE, &Parser::parse_example},
    {TokenType::SECURITY, &Parser::parse_security},
    {TokenType::COMPLEXITY, &Parser::parse_complexity},
    {TokenType::MODEL, &Parser::parse_model},
    {TokenType::FORMAT, &Parser::parse_format},
    {TokenType::VARIABLE, &Parser::parse_variable},
    {TokenType::OUTPUT_FORMAT, &Parser::parse_output_format},
    {TokenType::MAX_TOKENS, &Parser::parse_max_tokens},
    {TokenType::TEMPERATURE, &Parser::parse_temperature},
    {TokenType::PATTERN, &Parser::parse_pattern},
    {TokenType::STRUCTURE, &Parser::parse_structure},
    {TokenType::PROVIDER, &Parser::parse_provider}
};

// parsererror implementation
ParserError::ParserError(const std::string& message, const size_t line, const size_t column, std::string  error_code)
    : std::runtime_error("Parser error at line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + message),
      m_line(line), 
      m_column(column),
      m_error_code(std::move(error_code)) {}

// parser implementation
Parser::Parser(const std::string_view input) : m_lexer(input) {
    advance();
}

void Parser::advance() {
    m_current_token = m_lexer.next_token();
}

void Parser::synchronize() {
    // Advance tokens until we find one in the dispatch table or EOF
    while (m_current_token) {
        if (m_current_token->m_type == TokenType::NEWLINE) {
            advance();
            continue;
        }
        if (m_dispatch_table.find(m_current_token->m_type) != m_dispatch_table.end()) {
            return; // Found a valid directive to resume from
        }
        advance();
    }
}

std::vector<std::unique_ptr<QueryNode>> Parser::parse() {
    std::vector<std::unique_ptr<QueryNode>> nodes;
    m_error_reporter.clear();

    while (m_current_token) {
        if (m_current_token->m_type == TokenType::NEWLINE) {
            advance();
            continue;
        }

        // Validate the token is a known directive via dispatch table lookup
        const auto it = m_dispatch_table.find(m_current_token->m_type);
        if (it == m_dispatch_table.end()) {
            // Record the error and recover
            ParseDiagnostic diag;
            diag.line = m_current_token->m_line;
            diag.column = m_current_token->m_column;
            diag.error_code = parser_errors::UNEXPECTED_TOKEN;
            diag.message = "Expected directive keyword";
            diag.offending_token = m_current_token->m_value;
            diag.expected = "@directive";
            m_error_reporter.add_error(std::move(diag));

            advance(); // Skip the bad token
            synchronize();
            continue;
        }

        // Try to parse the directive, recovering on error
        try {
            nodes.push_back((this->*(it->second))());
        } catch (const ParserError& e) {
            ParseDiagnostic diag;
            diag.line = e.line();
            diag.column = e.column();
            diag.error_code = e.error_code();
            diag.message = e.what();
            m_error_reporter.add_error(std::move(diag));
            synchronize();
        } catch (const LexerError& e) {
            ParseDiagnostic diag;
            diag.line = e.line();
            diag.column = e.column();
            diag.error_code = parser_errors::INVALID_STRING;
            diag.message = e.what();
            m_error_reporter.add_error(std::move(diag));
            synchronize();
        }
    }

    // After parsing, throw if any errors accumulated
    m_error_reporter.throw_if_errors();

    return nodes;
}

std::string Parser::parse_string() {
    // skip whitespace tokens
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
    advance(); // move to the next token
    return value;
}

std::unique_ptr<QueryNode> Parser::parse_code_request() {
    const size_t line = m_current_token->m_line;
    const size_t column = m_current_token->m_column;
    
    advance(); // skip past @language token

    // get the language string
    std::string language = parse_string();

    // skip any newlines
    while (m_current_token && m_current_token->m_type == TokenType::NEWLINE) {
        advance();
    }

    // now expect @description
    if (!m_current_token || m_current_token->m_type != TokenType::DESCRIPTION) {
        throw ParserError(
            "Expected @description after @language",
            m_current_token ? m_current_token->m_line : line,
            m_current_token ? m_current_token->m_column : column
        );
    }

    advance(); // skip past @description token
    std::string description = parse_string();

    return std::make_unique<CodeRequestNode>(language, description);
}

std::unique_ptr<QueryNode> Parser::parse_context() {
    advance(); // skip @context
    std::string context = parse_string();
    return std::make_unique<ContextNode>(context);
}

std::unique_ptr<QueryNode> Parser::parse_test() {
    advance(); // skip @test
    std::vector<std::string> test_cases;

    // get the test case
    test_cases.push_back(parse_string());

    return std::make_unique<TestNode>(std::move(test_cases));
}

std::unique_ptr<QueryNode> Parser::parse_dependency() {
    advance(); // skip @dependency
    std::vector<std::string> dependencies;

    // get the dependency
    dependencies.push_back(parse_string());

    return std::make_unique<DependencyNode>(std::move(dependencies));
}

std::unique_ptr<QueryNode> Parser::parse_performance() {
    advance(); // skip @performance
    std::string requirement = parse_string();
    return std::make_unique<PerformanceNode>(requirement);
}

std::unique_ptr<QueryNode> Parser::parse_copyright() {
    advance(); // skip @copyright
    std::string license = parse_string();
    std::string owner = parse_string();
    return std::make_unique<CopyrightNode>(license, owner);
}

std::unique_ptr<QueryNode> Parser::parse_architecture() {
    advance(); // skip @architecture
    
    // Backup the current token to restore if needed
    const std::optional<Token> saved_token = m_current_token;
    
    // Check for layered format - first token should be the layer type
    if (m_current_token && m_current_token->m_type == TokenType::IDENTIFIER) {
        const std::string first_token = m_current_token->m_value;
        advance(); // consume the layer identifier
        
        // Convert to lowercase for consistent checking
        std::string lowercase_layer = first_token;
        std::ranges::transform(lowercase_layer, lowercase_layer.begin(),
                               [](const unsigned char c) {
                                   return std::tolower(c);
                               });
        
        // Check if this is a valid layer
        if (lowercase_layer == "foundation" || lowercase_layer == "component" || lowercase_layer == "interaction") {
            // This is the layered format
            PatternLayer layer = string_to_pattern_layer(lowercase_layer);
            
            // The Next token should be the pattern name
            std::string pattern_name = parse_string();
            
            // Check if we have parameters
            std::string parameters;
            if (m_current_token && m_current_token->m_type == TokenType::STRING) {
                parameters = parse_string();
            }
            
            return std::make_unique<ArchitectureNode>(layer, pattern_name, parameters);
        }
        
        // If we got here, it wasn't a valid layer
        // Restore token and continue with the standard format
        m_current_token = saved_token;
        
        // We'll just reparse the string as the regular format
        // The token position is already saved and restored above
    }
    
    // Old format - just a single string
    std::string architecture = parse_string();
    return std::make_unique<ArchitectureNode>(architecture);
}

std::unique_ptr<QueryNode> Parser::parse_constraint() {
    advance(); // skip @constraint
    std::string constraint = parse_string();
    return std::make_unique<ConstraintNode>(constraint);
}

std::unique_ptr<QueryNode> Parser::parse_example() {
    advance(); // skip @example
    std::string label = parse_string();
    std::string code = parse_string();
    return std::make_unique<ExampleNode>(label, code);
}

std::unique_ptr<QueryNode> Parser::parse_security() {
    advance(); // skip @security
    std::string requirement = parse_string();
    return std::make_unique<SecurityNode>(requirement);
}

std::unique_ptr<QueryNode> Parser::parse_complexity() {
    advance(); // skip @complexity
    std::string complexity = parse_string();
    return std::make_unique<ComplexityNode>(complexity);
}

std::unique_ptr<QueryNode> Parser::parse_model() {
    advance(); // skip @model
    std::string model_name = parse_string();
    return std::make_unique<ModelNode>(model_name);
}

std::unique_ptr<QueryNode> Parser::parse_format() {
    advance(); // skip @format
    std::string format_type = parse_string();
    return std::make_unique<FormatNode>(format_type);
}

std::unique_ptr<QueryNode> Parser::parse_variable() {
    advance(); // skip @variable
    std::string name = parse_string();
    std::string value = parse_string();
    return std::make_unique<VariableNode>(name, value);
}

// Model control directive parsers
std::unique_ptr<QueryNode> Parser::parse_output_format() {
    advance(); // skip @output_format
    std::string format = parse_string();
    return std::make_unique<OutputFormatNode>(format);
}

std::unique_ptr<QueryNode> Parser::parse_max_tokens() {
    advance(); // skip @max_tokens
    
    // Handle numeric values
    std::string token_value;
    if (m_current_token && m_current_token->m_type == TokenType::IDENTIFIER) {
        token_value = m_current_token->m_value;
        advance();
    } else {
        token_value = parse_string();
    }
    
    return std::make_unique<MaxTokensNode>(token_value);
}

std::unique_ptr<QueryNode> Parser::parse_temperature() {
    advance(); // skip @temperature
    
    // Handle numeric values
    std::string temp_value;
    if (m_current_token && m_current_token->m_type == TokenType::IDENTIFIER) {
        temp_value = m_current_token->m_value;
        advance();
    } else {
        temp_value = parse_string();
    }
    
    return std::make_unique<TemperatureNode>(temp_value);
}

// Project structure directive parsers
std::unique_ptr<QueryNode> Parser::parse_pattern() {
    advance(); // skip @pattern
    std::string pattern_desc = parse_string();
    return std::make_unique<PatternNode>(pattern_desc);
}

std::unique_ptr<QueryNode> Parser::parse_structure() {
    advance(); // skip @structure
    std::string structure_def = parse_string();
    return std::make_unique<StructureNode>(structure_def);
}

std::unique_ptr<QueryNode> Parser::parse_provider() {
    advance(); // skip @provider
    std::string provider_name = parse_string();
    return std::make_unique<ProviderNode>(provider_name);
}

} // namespace cql
