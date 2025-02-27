// MIT License
// Copyright (c) 2025 dbjwhs

#include <cctype>
#include "../../include/cql/lexer.hpp"

namespace cql {

// Convert token type to string (for debugging)
std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::LANGUAGE: return "LANGUAGE";
        case TokenType::DESCRIPTION: return "DESCRIPTION";
        case TokenType::CONTEXT: return "CONTEXT";
        case TokenType::TEST: return "TEST";
        case TokenType::DEPENDENCY: return "DEPENDENCY";
        case TokenType::PERFORMANCE: return "PERFORMANCE";
        case TokenType::COPYRIGHT: return "COPYRIGHT";
        case TokenType::ARCHITECTURE: return "ARCHITECTURE";
        case TokenType::CONSTRAINT: return "CONSTRAINT";
        case TokenType::EXAMPLE: return "EXAMPLE";
        case TokenType::SECURITY: return "SECURITY";
        case TokenType::COMPLEXITY: return "COMPLEXITY";
        case TokenType::MODEL: return "MODEL";
        case TokenType::FORMAT: return "FORMAT";
        case TokenType::VARIABLE: return "VARIABLE";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::STRING: return "STRING";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::END: return "END";
        default: return "UNKNOWN";
    }
}

// Token implementation
Token::Token(TokenType t, std::string v, size_t l, size_t c)
    : m_type(t), m_value(std::move(v)), m_line(l), m_column(c) {}

std::string Token::to_string() const {
    std::ostringstream oss;
    oss << "Token{type=" << token_type_to_string(m_type)
        << ", value='" << m_value << "'"
        << ", line=" << m_line
        << ", column=" << m_column << "}";
    return oss.str();
}

// Lexer implementation
Lexer::Lexer(std::string_view input)
    : m_input(input), m_current(0), m_line(1), m_column(1) {}

void Lexer::advance() {
    m_current++;
    m_column++;
}

void Lexer::skip_whitespace() {
    while (m_current < m_input.length() &&
           (std::isspace(m_input[m_current]) && m_input[m_current] != '\n')) {
        advance();
    }
}

std::optional<Token> Lexer::next_token() {
    skip_whitespace();

    if (m_current >= m_input.length()) {
        return std::nullopt;
    }

    if (m_input[m_current] == '@') {
        return lex_keyword();
    }

    if (m_input[m_current] == '"') {
        return lex_string();
    }

    if (m_input[m_current] == '\n') {
        auto token = Token(TokenType::NEWLINE, "\n", m_line, m_column);
        advance();
        m_line++;
        m_column = 1;
        return token;
    }

    return lex_identifier();
}

std::optional<Token> Lexer::lex_keyword() {
    advance(); // skip @
    std::string keyword;
    size_t start_column = m_column;

    while (m_current < m_input.length() && std::isalpha(m_input[m_current])) {
        keyword += m_input[m_current];
        advance();
    }

    TokenType type;
    if (keyword == "language") type = TokenType::LANGUAGE;
    else if (keyword == "description") type = TokenType::DESCRIPTION;
    else if (keyword == "context") type = TokenType::CONTEXT;
    else if (keyword == "test") type = TokenType::TEST;
    else if (keyword == "dependency") type = TokenType::DEPENDENCY;
    else if (keyword == "performance") type = TokenType::PERFORMANCE;
    else if (keyword == "copyright") type = TokenType::COPYRIGHT;
    // New directive types for Phase 2
    else if (keyword == "architecture") type = TokenType::ARCHITECTURE;
    else if (keyword == "constraint") type = TokenType::CONSTRAINT;
    else if (keyword == "example") type = TokenType::EXAMPLE;
    else if (keyword == "security") type = TokenType::SECURITY;
    else if (keyword == "complexity") type = TokenType::COMPLEXITY;
    else if (keyword == "model") type = TokenType::MODEL;
    else if (keyword == "format") type = TokenType::FORMAT;
    else if (keyword == "variable") type = TokenType::VARIABLE;
    else {
        throw LexerError("Unknown keyword: @" + keyword, m_line, start_column - 1);
    }

    return Token(type, keyword, m_line, start_column);
}

std::optional<Token> Lexer::lex_string() {
    if (m_current >= m_input.length() || m_input[m_current] != '"') {
        throw LexerError("Expected opening quote", m_line, m_column);
    }

    advance(); // skip opening quote
    std::string value;
    const size_t start_column = m_column;

    while (m_current < m_input.length() && m_input[m_current] != '"') {
        if (m_input[m_current] == '\\') {
            advance();
            if (m_current >= m_input.length()) {
                throw LexerError("Unterminated string escape sequence", m_line, m_column);
            }
            switch (m_input[m_current]) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default: 
                    throw LexerError("Invalid escape sequence", m_line, m_column);
            }
        } else {
            value += m_input[m_current];
        }
        advance();
    }

    if (m_current >= m_input.length()) {
        throw LexerError("Unterminated string", m_line, start_column);
    }

    advance(); // skip closing quote
    return Token(TokenType::STRING, value, m_line, start_column);
}

std::optional<Token> Lexer::lex_identifier() {
    std::string value;
    const size_t start_column = m_column;

    while (m_current < m_input.length() &&
           !std::isspace(m_input[m_current]) &&
           m_input[m_current] != '@') {
        value += m_input[m_current];
        advance();
    }

    return Token(TokenType::IDENTIFIER, value, m_line, start_column);
}

// LexerError implementation
LexerError::LexerError(const std::string& message, size_t line, size_t column)
    : std::runtime_error("Lexer error at line " + std::to_string(line) + 
                         ", column " + std::to_string(column) + ": " + message),
      m_line(line), m_column(column) {}

} // namespace cql