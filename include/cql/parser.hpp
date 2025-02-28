// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_PARSER_HPP
#define CQL_PARSER_HPP

#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>
#include "lexer.hpp"
#include "nodes.hpp"

namespace cql {

/**
 * Standard parser error codes
 * Using the format PAR-XXX where XXX is a numeric code
 * 100-199: Token-related errors
 * 200-299: Syntax-related errors
 * 300-399: Structure-related errors
 */
namespace parser_errors {
    // General parsing errors
    inline constexpr char GENERAL_ERROR[] = "PAR-001";
    
    // Token-related errors
    inline constexpr char UNEXPECTED_TOKEN[] = "PAR-101";
    inline constexpr char MISSING_TOKEN[] = "PAR-102";
    inline constexpr char UNEXPECTED_END[] = "PAR-103";
    
    // Syntax-related errors
    inline constexpr char INVALID_DIRECTIVE[] = "PAR-201";
    inline constexpr char INVALID_STRING[] = "PAR-202";
    
    // Structure-related errors
    inline constexpr char INVALID_STRUCTURE[] = "PAR-301";
}

/**
 * parser for building the abstract syntax tree (ast)
 * converts tokens from the lexer into a tree of querynode objects
 */
class Parser {
public:
    explicit Parser(const std::string_view input);
    
    // parse the input and build the ast
    std::vector<std::unique_ptr<QueryNode>> parse();

private:
    Lexer m_lexer;
    std::optional<Token> m_current_token;

    // move to the next token in the input
    void advance();
    
    // parse a string token
    std::string parse_string();
    
    // parse node types
    std::unique_ptr<QueryNode> parse_code_request();
    std::unique_ptr<QueryNode> parse_context();
    std::unique_ptr<QueryNode> parse_test();
    std::unique_ptr<QueryNode> parse_dependency();
    std::unique_ptr<QueryNode> parse_performance();
    std::unique_ptr<QueryNode> parse_copyright();
    
    // phase 2 directive parsers
    std::unique_ptr<QueryNode> parse_architecture();
    std::unique_ptr<QueryNode> parse_constraint();
    std::unique_ptr<QueryNode> parse_example();
    std::unique_ptr<QueryNode> parse_security();
    std::unique_ptr<QueryNode> parse_complexity();
    std::unique_ptr<QueryNode> parse_model();
    std::unique_ptr<QueryNode> parse_format();
    std::unique_ptr<QueryNode> parse_variable();
};

/**
 * Custom exception class for parser errors with location information
 * 
 * This specialized exception provides detailed information about parsing errors,
 * including the location in the source code where the error occurred and a
 * machine-readable error code.
 */
class ParserError : public std::runtime_error {
public:
    /**
     * Create a new parser error
     * @param message The error message
     * @param line The line number where the error occurred
     * @param column The column number where the error occurred
     * @param error_code The error code for this parsing error
     */
    ParserError(
        const std::string& message,
        size_t line,
        size_t column,
        const std::string& error_code = parser_errors::GENERAL_ERROR);
    
    /**
     * Get the line number where the error occurred
     * @return The line number
     */
    [[nodiscard]] size_t line() const { return m_line; }
    
    /**
     * Get the column number where the error occurred
     * @return The column number
     */
    [[nodiscard]] size_t column() const { return m_column; }
    
    /**
     * Get the error code for this parsing error
     * @return The error code string
     */
    [[nodiscard]] const std::string& error_code() const { return m_error_code; }
    
    /**
     * Create a formatted error message including the error code and location
     * @return Formatted error message with code and location
     */
    [[nodiscard]] std::string formatted_message() const {
        return "[" + m_error_code + "] " + what() + " at line " + 
               std::to_string(m_line) + ", column " + std::to_string(m_column);
    }
    
private:
    size_t m_line;
    size_t m_column;
    std::string m_error_code;
};

} // namespace cql

#endif // cql_parser_hpp
