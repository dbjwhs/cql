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
 * Token types for the DSL grammar
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
 * Convert token type to string representation (for debugging)
 */
std::string token_type_to_string(TokenType type);

/**
 * Token structure for lexical analysis
 * Contains type, value, and source location information
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
 * Lexical analyzer (lexer) for tokenizing input
 * Converts raw text into a stream of tokens for the parser
 */
class Lexer {
public:
    explicit Lexer(std::string_view input);
    
    // Get the next token from the input
    std::optional<Token> next_token();
    
    // Get the current line number (for error reporting)
    [[nodiscard]] size_t current_line() const { return m_line; }
    
    // Get the current column number (for error reporting)
    [[nodiscard]] size_t current_column() const { return m_column; }

private:
    std::string_view m_input;
    size_t m_current;
    size_t m_line;
    size_t m_column;

    // Move to the next character in the input
    void advance();
    
    // Skip whitespace characters (except newlines)
    void skip_whitespace();
    
    // Parse a keyword token (starting with @)
    std::optional<Token> lex_keyword();
    
    // Parse a string token (enclosed in quotes)
    std::optional<Token> lex_string();
    
    // Parse an identifier token
    std::optional<Token> lex_identifier();
};

/**
 * Custom exception class for lexer errors with location information
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

#endif // CQL_LEXER_HPP
