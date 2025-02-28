// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_LEXER_HPP
#define CQL_LEXER_HPP

#include <string>
#include <string_view>
#include <optional>
#include <stdexcept>
#include <sstream>

namespace cql {

/**
 * token types for the dsl grammar
 */
enum class TokenType {
    LANGUAGE,       // @language
    DESCRIPTION,    // @description
    CONTEXT,        // @context
    TEST,           // @test
    DEPENDENCY,     // @dependency
    PERFORMANCE,    // @performance
    COPYRIGHT,      // @copyright
    ARCHITECTURE,   // @architecture
    CONSTRAINT,     // @constraint
    EXAMPLE,        // @example
    SECURITY,       // @security
    COMPLEXITY,     // @complexity
    MODEL,          // @model
    FORMAT,         // @format
    VARIABLE,       // @variable
    IDENTIFIER,     // any text
    STRING,         // "quoted text"
    NEWLINE,        // \n
    END             // end of input
};

/**
 * convert token type to string representation (for debugging)
 */
std::string token_type_to_string(TokenType type);

/**
 * token structure for lexical analysis
 * contains type, value, and source location information
 */
struct Token {
    TokenType m_type;
    std::string m_value;
    size_t m_line;
    size_t m_column;

    Token(TokenType t, std::string v, size_t l, size_t c);
    
    [[nodiscard]] std::string to_string() const;
};

/**
 * lexical analyzer (lexer) for tokenizing input
 * converts raw text into a stream of tokens for the parser
 */
class Lexer {
public:
    explicit Lexer(std::string_view input);
    
    // get the next token from the input
    std::optional<Token> next_token();
    
    // get the current line number (for error reporting)
    [[nodiscard]] size_t current_line() const { return m_line; }
    
    // get the current column number (for error reporting)
    [[nodiscard]] size_t current_column() const { return m_column; }

private:
    std::string_view m_input;
    size_t m_current;
    size_t m_line;
    size_t m_column;

    // move to the next character in the input
    void advance();
    
    // skip whitespace characters (except newlines)
    void skip_whitespace();
    
    // parse a keyword token (starting with @)
    std::optional<Token> lex_keyword();
    
    // parse a string token (enclosed in quotes)
    std::optional<Token> lex_string();
    
    // parse an identifier token
    std::optional<Token> lex_identifier();
};

/**
 * custom exception class for lexer errors with location information
 */
class LexerError : public std::runtime_error {
public:
    LexerError(const std::string& message, size_t line, size_t column);
    
    [[nodiscard]] size_t line() const { return m_line; }
    [[nodiscard]] size_t column() const { return m_column; }
    
private:
    size_t m_line;
    size_t m_column;
};

} // namespace cql

#endif // cql_lexer_hpp
