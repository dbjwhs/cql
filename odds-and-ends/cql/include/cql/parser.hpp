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
 * custom exception class for parser errors with location information
 */
class ParserError : public std::runtime_error {
public:
    ParserError(const std::string& message, size_t line, size_t column);
    
    [[nodiscard]] size_t line() const { return m_line; }
    [[nodiscard]] size_t column() const { return m_column; }
    
private:
    size_t m_line;
    size_t m_column;
};

} // namespace cql

#endif // cql_parser_hpp
