// MIT License
// Copyright (c) 2025 dbjwhs

#include <cctype>
#include "../../include/cql/lexer.hpp"

namespace cql {

// convert token type to string (for debugging)
std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::LANGUAGE:      return "LANGUAGE";
        case TokenType::DESCRIPTION:   return "DESCRIPTION";
        case TokenType::CONTEXT:       return "CONTEXT";
        case TokenType::TEST:          return "TEST";
        case TokenType::DEPENDENCY:    return "DEPENDENCY";
        case TokenType::PERFORMANCE:   return "PERFORMANCE";
        case TokenType::COPYRIGHT:     return "COPYRIGHT";
        case TokenType::ARCHITECTURE:  return "ARCHITECTURE";
        case TokenType::CONSTRAINT:    return "CONSTRAINT";
        case TokenType::EXAMPLE:       return "EXAMPLE";
        case TokenType::SECURITY:      return "SECURITY";
        case TokenType::COMPLEXITY:    return "COMPLEXITY";
        case TokenType::MODEL:         return "MODEL";
        case TokenType::FORMAT:        return "FORMAT";
        case TokenType::VARIABLE:      return "VARIABLE";
        case TokenType::OUTPUT_FORMAT: return "OUTPUT_FORMAT";
        case TokenType::MAX_TOKENS:    return "MAX_TOKENS";
        case TokenType::TEMPERATURE:   return "TEMPERATURE";
        case TokenType::PATTERN:       return "PATTERN";
        case TokenType::STRUCTURE:     return "STRUCTURE";
        case TokenType::IDENTIFIER:    return "IDENTIFIER";
        case TokenType::STRING:        return "STRING";
        case TokenType::NEWLINE:       return "NEWLINE";
        case TokenType::END:           return "END";
        default: return "UNKNOWN";
    }
}

// token implementation
Token::Token(TokenType t, std::string v, const size_t l, const size_t c)
    : m_type(t), m_value(std::move(v)), m_line(l), m_column(c) {}

std::string Token::to_string() const {
    std::ostringstream oss;
    oss << "Token{type=" << token_type_to_string(m_type)
        << ", value='" << m_value << "'"
        << ", line=" << m_line
        << ", column=" << m_column << "}";
    return oss.str();
}

// lexer implementation
Lexer::Lexer(const std::string_view input) : m_input(input), m_current(0), m_line(1), m_column(1) {}

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

void Lexer::skip_trailing_whitespace() {
    while (m_current < m_input.length() && 
           std::isspace(m_input[m_current]) && 
           m_input[m_current] != '\n') {
        advance();
    }
}

std::optional<Token> Lexer::next_token() {
    skip_whitespace();

    if (m_current >= m_input.length()) {
        return std::nullopt;
    }
    
    // Handle comment lines starting with #
    if (m_input[m_current] == '#') {
        // Skip everything until the end of line
        while (m_current < m_input.length() && m_input[m_current] != '\n') {
            advance();
        }
        
        // If we reached a newline, process it but don't return a token for it
        if (m_current < m_input.length() && m_input[m_current] == '\n') {
            advance();
            m_line++;
            m_column = 1;
        }
        
        // Continue to the next token instead of returning the newline
        return next_token();
    }
    
    // Handle C++ style line comments (//...)
    if (m_current + 1 < m_input.length() && m_input[m_current] == '/' && m_input[m_current + 1] == '/') {
        // Skip everything until the end of line
        while (m_current < m_input.length() && m_input[m_current] != '\n') {
            advance();
        }
        
        // If we reached a newline, process it but don't return a token for it
        if (m_current < m_input.length() && m_input[m_current] == '\n') {
            advance();
            m_line++;
            m_column = 1;
        }
        
        // Continue to the next token instead of returning the newline
        return next_token();
    }
    
    // Handle C style block comments (/* ... */)
    if (m_current + 1 < m_input.length() && m_input[m_current] == '/' && m_input[m_current + 1] == '*') {
        // Skip the opening /*
        advance(); // skip /
        advance(); // skip *
        
        // Continue until we find the closing */
        while (m_current + 1 < m_input.length() && 
              !(m_input[m_current] == '*' && m_input[m_current + 1] == '/')) {
            if (m_input[m_current] == '\n') {
                m_line++;
                m_column = 0; // Will be incremented to 1 by advance()
            }
            advance();
        }
        
        // Skip the closing */ if we found it
        if (m_current + 1 < m_input.length() && 
            m_input[m_current] == '*' && m_input[m_current + 1] == '/') {
            advance(); // skip *
            advance(); // skip /
        }
        
        // Continue to the next token
        return next_token();
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

    while (m_current < m_input.length() && (std::isalpha(m_input[m_current]) || m_input[m_current] == '_')) {
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
    else if (keyword == "architecture") type = TokenType::ARCHITECTURE;
    else if (keyword == "constraint") type = TokenType::CONSTRAINT;
    else if (keyword == "example") type = TokenType::EXAMPLE;
    else if (keyword == "security") type = TokenType::SECURITY;
    else if (keyword == "complexity") type = TokenType::COMPLEXITY;
    else if (keyword == "model") type = TokenType::MODEL;
    else if (keyword == "format") type = TokenType::FORMAT;
    else if (keyword == "variable") type = TokenType::VARIABLE;
    else if (keyword == "output_format") type = TokenType::OUTPUT_FORMAT;
    else if (keyword == "max_tokens") type = TokenType::MAX_TOKENS;
    else if (keyword == "temperature") type = TokenType::TEMPERATURE;
    else if (keyword == "pattern") type = TokenType::PATTERN;
    else if (keyword == "structure") type = TokenType::STRUCTURE;
    else {
        throw LexerError("Unknown keyword: @" + keyword, m_line, start_column - 1);
    }
    
    skip_trailing_whitespace();

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
    skip_trailing_whitespace();
    
    return Token(TokenType::STRING, value, m_line, start_column);
}

std::optional<Token> Lexer::lex_identifier() {
    std::string value;
    const size_t start_column = m_column;

    while (m_current < m_input.length() &&
           !std::isspace(m_input[m_current]) &&
           m_input[m_current] != '@' &&
           m_input[m_current] != '#' &&
           !(m_current + 1 < m_input.length() && m_input[m_current] == '/' && 
             (m_input[m_current + 1] == '/' || m_input[m_current + 1] == '*'))) {
        value += m_input[m_current];
        advance();
    }
    
    skip_trailing_whitespace();

    return Token(TokenType::IDENTIFIER, value, m_line, start_column);
}

// LexerError implementation
LexerError::LexerError(const std::string& message, size_t line, size_t column)
    : std::runtime_error("Lexer error at line " + std::to_string(line) + 
                         ", column " + std::to_string(column) + ": " + message),
      m_line(line), m_column(column) {}

} // namespace cql
