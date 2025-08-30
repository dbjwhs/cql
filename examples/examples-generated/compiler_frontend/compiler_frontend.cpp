// MIT License
// Copyright (c) 2025 dbjwhs

#include "compiler_frontend.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace compiler_frontend {

// Token implementation
std::string Token::to_string() const {
    return std::format("{}: '{}' at {}", 
                      static_cast<int>(type), value, location.to_string());
}

// CompilerError implementation
std::string CompilerError::format() const {
    const char* level_str = "UNKNOWN";
    switch (m_level) {
        case Level::INFO: level_str = "INFO"; break;
        case Level::WARNING: level_str = "WARNING"; break;
        case Level::ERROR: level_str = "ERROR"; break;
        case Level::FATAL: level_str = "FATAL"; break;
    }
    
    return std::format("{}: {} at {}", level_str, m_message, m_location.to_string());
}

// SymbolTable implementation
void SymbolTable::push_scope() {
    m_scopes.emplace_back();
}

void SymbolTable::pop_scope() {
    if (!m_scopes.empty()) {
        m_scopes.pop_back();
    }
}

bool SymbolTable::declare(const Symbol& symbol) {
    if (m_scopes.empty()) {
        return false;
    }
    
    auto& current_scope = m_scopes.back();
    if (current_scope.contains(symbol.name)) {
        return false; // Symbol already declared in current scope
    }
    
    current_scope[symbol.name] = symbol;
    return true;
}

std::optional<Symbol> SymbolTable::lookup(const std::string& name) const {
    // Search from innermost to outermost scope
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        if (auto found = it->find(name); found != it->end()) {
            return found->second;
        }
    }
    return std::nullopt;
}

bool SymbolTable::is_declared_in_current_scope(const std::string& name) const {
    if (m_scopes.empty()) {
        return false;
    }
    return m_scopes.back().contains(name);
}

// Lexer implementation
Lexer::Lexer(std::string source, std::string filename)
    : m_source(std::move(source)), m_filename(std::move(filename)) {
    initialize_token_rules();
}

void Lexer::initialize_token_rules() {
    // Keywords (highest priority)
    m_token_rules.push_back({TokenType::FUNCTION, std::regex(R"(\bfunction\b)"), 1});
    m_token_rules.push_back({TokenType::IF, std::regex(R"(\bif\b)"), 1});
    m_token_rules.push_back({TokenType::ELSE, std::regex(R"(\belse\b)"), 1});
    m_token_rules.push_back({TokenType::WHILE, std::regex(R"(\bwhile\b)"), 1});
    m_token_rules.push_back({TokenType::FOR, std::regex(R"(\bfor\b)"), 1});
    m_token_rules.push_back({TokenType::RETURN, std::regex(R"(\breturn\b)"), 1});
    m_token_rules.push_back({TokenType::VAR, std::regex(R"(\bvar\b)"), 1});
    m_token_rules.push_back({TokenType::CONST, std::regex(R"(\bconst\b)"), 1});
    m_token_rules.push_back({TokenType::TRUE, std::regex(R"(\btrue\b)"), 1});
    m_token_rules.push_back({TokenType::FALSE, std::regex(R"(\bfalse\b)"), 1});
    
    // Types
    m_token_rules.push_back({TokenType::INT, std::regex(R"(\bint\b)"), 1});
    m_token_rules.push_back({TokenType::FLOAT_TYPE, std::regex(R"(\bfloat\b)"), 1});
    m_token_rules.push_back({TokenType::STRING_TYPE, std::regex(R"(\bstring\b)"), 1});
    m_token_rules.push_back({TokenType::BOOL_TYPE, std::regex(R"(\bbool\b)"), 1});
    
    // Multi-character operators (higher priority than single-char)
    m_token_rules.push_back({TokenType::ARROW, std::regex(R"(->)"), 2});
    m_token_rules.push_back({TokenType::EQUAL, std::regex(R"(==)"), 2});
    m_token_rules.push_back({TokenType::NOT_EQUAL, std::regex(R"(!=)"), 2});
    m_token_rules.push_back({TokenType::LESS_EQUAL, std::regex(R"(<=)"), 2});
    m_token_rules.push_back({TokenType::GREATER_EQUAL, std::regex(R"(>=)"), 2});
    m_token_rules.push_back({TokenType::LOGICAL_AND, std::regex(R"(&&)"), 2});
    m_token_rules.push_back({TokenType::LOGICAL_OR, std::regex(R"(\|\|)"), 2});
    
    // Single-character operators
    m_token_rules.push_back({TokenType::PLUS, std::regex(R"(\+)"), 3});
    m_token_rules.push_back({TokenType::MINUS, std::regex(R"(-)"), 3});
    m_token_rules.push_back({TokenType::MULTIPLY, std::regex(R"(\*)"), 3});
    m_token_rules.push_back({TokenType::DIVIDE, std::regex(R"(/)"), 3});
    m_token_rules.push_back({TokenType::MODULO, std::regex(R"(%)"), 3});
    m_token_rules.push_back({TokenType::ASSIGN, std::regex(R"(=)"), 3});
    m_token_rules.push_back({TokenType::LESS, std::regex(R"(<)"), 3});
    m_token_rules.push_back({TokenType::GREATER, std::regex(R"(>)"), 3});
    m_token_rules.push_back({TokenType::LOGICAL_NOT, std::regex(R"(!)"), 3});
    
    // Punctuation
    m_token_rules.push_back({TokenType::LEFT_PAREN, std::regex(R"(\()"), 4});
    m_token_rules.push_back({TokenType::RIGHT_PAREN, std::regex(R"(\))"), 4});
    m_token_rules.push_back({TokenType::LEFT_BRACE, std::regex(R"(\{)"), 4});
    m_token_rules.push_back({TokenType::RIGHT_BRACE, std::regex(R"(\})"), 4});
    m_token_rules.push_back({TokenType::LEFT_BRACKET, std::regex(R"(\[)"), 4});
    m_token_rules.push_back({TokenType::RIGHT_BRACKET, std::regex(R"(\])"), 4});
    m_token_rules.push_back({TokenType::COMMA, std::regex(R"(,)"), 4});
    m_token_rules.push_back({TokenType::SEMICOLON, std::regex(R"(;)"), 4});
    m_token_rules.push_back({TokenType::COLON, std::regex(R"(:)"), 4});
    
    // Literals
    m_token_rules.push_back({TokenType::FLOAT, std::regex(R"(\d+\.\d+)"), 5});
    m_token_rules.push_back({TokenType::INTEGER, std::regex(R"(\d+)"), 6});
    m_token_rules.push_back({TokenType::STRING, std::regex(R"("([^"\\]|\\.)*")"), 5});
    m_token_rules.push_back({TokenType::IDENTIFIER, std::regex(R"([a-zA-Z_][a-zA-Z0-9_]*)"), 7});
    
    // Newlines
    m_token_rules.push_back({TokenType::NEWLINE, std::regex(R"(\n)"), 8});
    
    // Sort by priority (lower numbers = higher priority)
    std::sort(m_token_rules.begin(), m_token_rules.end(),
              [](const TokenRule& a, const TokenRule& b) { return a.priority < b.priority; });
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (m_position < m_source.length()) {
        skip_whitespace();
        
        if (m_position >= m_source.length()) {
            break;
        }
        
        // Skip comments
        if (m_position + 1 < m_source.length() && 
            m_source[m_position] == '/' && m_source[m_position + 1] == '/') {
            skip_comment();
            continue;
        }
        
        auto token = next_token();
        if (token) {
            tokens.push_back(std::move(*token));
        } else {
            // Error recovery: skip invalid character
            add_error(CompilerError::Level::ERROR, 
                     std::format("Unexpected character: '{}'", m_source[m_position]));
            advance();
        }
    }
    
    tokens.emplace_back(TokenType::EOF_TOKEN, "", current_location());
    return tokens;
}

std::optional<Token> Lexer::next_token() {
    if (m_position >= m_source.length()) {
        return std::nullopt;
    }
    
    // Try each token rule in order of priority
    for (const auto& rule : m_token_rules) {
        std::smatch match;
        std::string remaining = m_source.substr(m_position);
        
        if (std::regex_search(remaining, match, rule.pattern) && match.position() == 0) {
            std::string value = match.str();
            auto location = current_location();
            
            advance(value.length());
            
            // Handle special cases
            if (rule.type == TokenType::IDENTIFIER) {
                // Check if it's actually a boolean literal
                if (value == "true") {
                    return Token(TokenType::TRUE, value, location);
                } else if (value == "false") {
                    return Token(TokenType::FALSE, value, location);
                }
            }
            
            return Token(rule.type, value, location);
        }
    }
    
    return std::nullopt;
}

SourceLocation Lexer::current_location() const {
    return {m_line, m_column, m_position, m_filename};
}

void Lexer::advance(size_t count) {
    for (size_t i = 0; i < count && m_position < m_source.length(); ++i) {
        if (m_source[m_position] == '\n') {
            ++m_line;
            m_column = 1;
        } else {
            ++m_column;
        }
        ++m_position;
    }
}

void Lexer::skip_whitespace() {
    while (m_position < m_source.length() && 
           std::isspace(m_source[m_position]) && 
           m_source[m_position] != '\n') {
        advance();
    }
}

void Lexer::skip_comment() {
    while (m_position < m_source.length() && m_source[m_position] != '\n') {
        advance();
    }
}

void Lexer::add_error(CompilerError::Level level, const std::string& message) {
    m_errors.emplace_back(level, message, current_location());
}

// Parser implementation
Parser::Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)) {}

std::unique_ptr<Program> Parser::parse() {
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!is_at_end()) {
        if (check(TokenType::NEWLINE)) {
            advance();
            continue;
        }
        
        auto stmt = parse_statement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    SourceLocation loc = statements.empty() ? 
        SourceLocation{1, 1, 0, ""} : 
        statements[0]->location();
    
    return std::make_unique<Program>(std::move(statements), loc);
}

std::unique_ptr<Statement> Parser::parse_statement() {
    if (match({TokenType::FUNCTION})) {
        return parse_function_declaration();
    }
    if (match({TokenType::VAR, TokenType::CONST})) {
        return parse_variable_declaration();
    }
    if (match({TokenType::IF})) {
        return parse_if_statement();
    }
    if (match({TokenType::WHILE})) {
        return parse_while_statement();
    }
    if (match({TokenType::RETURN})) {
        return parse_return_statement();
    }
    if (check(TokenType::LEFT_BRACE)) {
        return parse_block_statement();
    }
    
    return parse_expression_statement();
}

std::unique_ptr<Statement> Parser::parse_function_declaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    
    (void)consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    
    std::vector<Parameter> parameters;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            Token param_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            (void)consume(TokenType::COLON, "Expected ':' after parameter name");
            Token param_type = match({TokenType::INT, TokenType::FLOAT_TYPE, TokenType::STRING_TYPE, TokenType::BOOL_TYPE}) ?
                previous() : consume(TokenType::IDENTIFIER, "Expected parameter type");
            
            parameters.push_back({param_name.value, param_type.value, param_name.location});
        } while (match({TokenType::COMMA}));
    }
    
    (void)consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    (void)consume(TokenType::ARROW, "Expected '->' after parameters");
    
    Token return_type = match({TokenType::INT, TokenType::FLOAT_TYPE, TokenType::STRING_TYPE, TokenType::BOOL_TYPE}) ?
        previous() : consume(TokenType::IDENTIFIER, "Expected return type");
    
    auto body = parse_block_statement();
    
    return std::make_unique<FunctionDecl>(name.value, std::move(parameters),
                                         return_type.value, std::move(body), name.location);
}

std::unique_ptr<Statement> Parser::parse_variable_declaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    
    std::optional<std::string> type;
    if (match({TokenType::COLON})) {
        Token type_token = match({TokenType::INT, TokenType::FLOAT_TYPE, TokenType::STRING_TYPE, TokenType::BOOL_TYPE}) ?
            previous() : consume(TokenType::IDENTIFIER, "Expected type after ':'");
        type = type_token.value;
    }
    
    std::unique_ptr<Expression> initializer;
    if (match({TokenType::ASSIGN})) {
        initializer = parse_expression();
    }
    
    (void)consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return std::make_unique<VariableDecl>(name.value, type, std::move(initializer), name.location);
}

std::unique_ptr<Statement> Parser::parse_if_statement() {
    (void)consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'");
    auto condition = parse_expression();
    (void)consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition");
    
    auto then_stmt = parse_statement();
    
    std::unique_ptr<Statement> else_stmt;
    if (match({TokenType::ELSE})) {
        else_stmt = parse_statement();
    }
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(then_stmt), std::move(else_stmt));
}

std::unique_ptr<Statement> Parser::parse_while_statement() {
    (void)consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'");
    auto condition = parse_expression();
    (void)consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition");
    
    auto body = parse_statement();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::parse_return_statement() {
    auto location = previous().location;
    
    std::unique_ptr<Expression> value;
    if (!check(TokenType::SEMICOLON) && !check(TokenType::NEWLINE)) {
        value = parse_expression();
    }
    
    (void)consume(TokenType::SEMICOLON, "Expected ';' after return value");
    
    return std::make_unique<ReturnStmt>(std::move(value), location);
}

std::unique_ptr<Statement> Parser::parse_block_statement() {
    Token left_brace = consume(TokenType::LEFT_BRACE, "Expected '{'");
    auto location = left_brace.location;
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        if (check(TokenType::NEWLINE)) {
            advance();
            continue;
        }
        
        auto stmt = parse_statement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    (void)consume(TokenType::RIGHT_BRACE, "Expected '}' after block");
    
    return std::make_unique<BlockStmt>(std::move(statements), location);
}

std::unique_ptr<Statement> Parser::parse_expression_statement() {
    auto expr = parse_expression();
    (void)consume(TokenType::SEMICOLON, "Expected ';' after expression");
    
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_logical_or();
}

std::unique_ptr<Expression> Parser::parse_logical_or() {
    auto expr = parse_logical_and();
    
    while (match({TokenType::LOGICAL_OR})) {
        Token operator_token = previous();
        auto right = parse_logical_and();
        expr = std::make_unique<BinaryExpr>(std::move(expr), operator_token, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_logical_and() {
    auto expr = parse_equality();
    
    while (match({TokenType::LOGICAL_AND})) {
        Token operator_token = previous();
        auto right = parse_equality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), operator_token, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_equality() {
    auto expr = parse_comparison();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        Token operator_token = previous();
        auto right = parse_comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), operator_token, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_comparison() {
    auto expr = parse_term();
    
    while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
        Token operator_token = previous();
        auto right = parse_term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), operator_token, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_term() {
    auto expr = parse_factor();
    
    while (match({TokenType::MINUS, TokenType::PLUS})) {
        Token operator_token = previous();
        auto right = parse_factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), operator_token, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_factor() {
    auto expr = parse_unary();
    
    while (match({TokenType::DIVIDE, TokenType::MULTIPLY, TokenType::MODULO})) {
        Token operator_token = previous();
        auto right = parse_unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), operator_token, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_unary() {
    if (match({TokenType::LOGICAL_NOT, TokenType::MINUS})) {
        Token operator_token = previous();
        auto right = parse_unary();
        return std::make_unique<UnaryExpr>(operator_token, std::move(right));
    }
    
    return parse_call();
}

std::unique_ptr<Expression> Parser::parse_call() {
    auto expr = parse_primary();
    
    while (true) {
        if (match({TokenType::LEFT_PAREN})) {
            std::vector<std::unique_ptr<Expression>> arguments;
            
            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    arguments.push_back(parse_expression());
                } while (match({TokenType::COMMA}));
            }
            
            (void)consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(arguments));
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_primary() {
    if (match({TokenType::TRUE})) {
        return std::make_unique<LiteralExpr>(true, previous().location);
    }
    
    if (match({TokenType::FALSE})) {
        return std::make_unique<LiteralExpr>(false, previous().location);
    }
    
    if (match({TokenType::INTEGER})) {
        Token token = previous();
        int64_t value = std::stoll(token.value);
        return std::make_unique<LiteralExpr>(value, token.location);
    }
    
    if (match({TokenType::FLOAT})) {
        Token token = previous();
        double value = std::stod(token.value);
        return std::make_unique<LiteralExpr>(value, token.location);
    }
    
    if (match({TokenType::STRING})) {
        Token token = previous();
        // Remove quotes from string literal
        std::string value = token.value.substr(1, token.value.length() - 2);
        return std::make_unique<LiteralExpr>(value, token.location);
    }
    
    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<IdentifierExpr>(previous().value, previous().location);
    }
    
    if (match({TokenType::LEFT_PAREN})) {
        auto expr = parse_expression();
        (void)consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    add_error(CompilerError::Level::ERROR, "Expected expression");
    // Return a dummy literal for error recovery
    return std::make_unique<LiteralExpr>(0, current_location());
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return false;
    return peek().type == type;
}

const Token& Parser::advance() {
    if (!is_at_end()) ++m_current;
    return previous();
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::EOF_TOKEN;
}

const Token& Parser::peek() const {
    return m_tokens[m_current];
}

const Token& Parser::previous() const {
    return m_tokens[m_current - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    add_error(CompilerError::Level::ERROR, message);
    // Return a dummy token for error recovery instead of throwing
    return Token(TokenType::INVALID, "", current_location());
}

SourceLocation Parser::current_location() const {
    if (is_at_end()) {
        return m_tokens.empty() ? SourceLocation{} : m_tokens.back().location;
    }
    return peek().location;
}

void Parser::synchronize() {
    advance();
    
    while (!is_at_end()) {
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::FUNCTION:
            case TokenType::VAR:
            case TokenType::CONST:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

void Parser::add_error(CompilerError::Level level, const std::string& message) {
    auto location = current_location();
    m_errors.emplace_back(level, message, location);
}

// PrettyPrintVisitor implementation
void PrettyPrintVisitor::visit_binary_expr(BinaryExpr& node) {
    m_output << "(";
    const_cast<Expression&>(node.left()).accept(*this);
    m_output << " " << node.operator_token().value << " ";
    const_cast<Expression&>(node.right()).accept(*this);
    m_output << ")";
}

void PrettyPrintVisitor::visit_unary_expr(UnaryExpr& node) {
    m_output << "(" << node.operator_token().value;
    const_cast<Expression&>(node.operand()).accept(*this);
    m_output << ")";
}

void PrettyPrintVisitor::visit_literal_expr(LiteralExpr& node) {
    std::visit([this](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::string>) {
            m_output << "\"" << value << "\"";
        } else if constexpr (std::is_same_v<T, bool>) {
            m_output << (value ? "true" : "false");
        } else {
            m_output << value;
        }
    }, node.value());
}

void PrettyPrintVisitor::visit_identifier_expr(IdentifierExpr& node) {
    m_output << node.name();
}

void PrettyPrintVisitor::visit_call_expr(CallExpr& node) {
    const_cast<Expression&>(node.callee()).accept(*this);
    m_output << "(";
    bool first = true;
    for (const auto& arg : node.arguments()) {
        if (!first) m_output << ", ";
        arg->accept(*this);
        first = false;
    }
    m_output << ")";
}

void PrettyPrintVisitor::visit_expression_stmt(ExpressionStmt& node) {
    print_indent();
    const_cast<Expression&>(node.expression()).accept(*this);
    m_output << ";\n";
}

void PrettyPrintVisitor::visit_variable_decl(VariableDecl& node) {
    print_indent();
    m_output << "var " << node.name();
    if (node.type()) {
        m_output << ": " << *node.type();
    }
    if (node.initializer()) {
        m_output << " = ";
        node.initializer()->accept(*this);
    }
    m_output << ";\n";
}

void PrettyPrintVisitor::visit_function_decl(FunctionDecl& node) {
    print_indent();
    m_output << "function " << node.name() << "(";
    
    bool first = true;
    for (const auto& param : node.parameters()) {
        if (!first) m_output << ", ";
        m_output << param.name << ": " << param.type;
        first = false;
    }
    
    m_output << ") -> " << node.return_type() << " ";
    const_cast<Statement&>(node.body()).accept(*this);
}

void PrettyPrintVisitor::visit_if_stmt(IfStmt& node) {
    print_indent();
    m_output << "if (";
    const_cast<Expression&>(node.condition()).accept(*this);
    m_output << ") ";
    const_cast<Statement&>(node.then_stmt()).accept(*this);
    
    if (node.else_stmt()) {
        print_indent();
        m_output << "else ";
        node.else_stmt()->accept(*this);
    }
}

void PrettyPrintVisitor::visit_while_stmt(WhileStmt& node) {
    print_indent();
    m_output << "while (";
    const_cast<Expression&>(node.condition()).accept(*this);
    m_output << ") ";
    const_cast<Statement&>(node.body()).accept(*this);
}

void PrettyPrintVisitor::visit_return_stmt(ReturnStmt& node) {
    print_indent();
    m_output << "return";
    if (node.value()) {
        m_output << " ";
        node.value()->accept(*this);
    }
    m_output << ";\n";
}

void PrettyPrintVisitor::visit_block_stmt(BlockStmt& node) {
    m_output << "{\n";
    increase_indent();
    
    for (const auto& stmt : node.statements()) {
        stmt->accept(*this);
    }
    
    decrease_indent();
    print_indent();
    m_output << "}\n";
}

void PrettyPrintVisitor::visit_program(Program& node) {
    for (const auto& stmt : node.statements()) {
        stmt->accept(*this);
    }
}

void PrettyPrintVisitor::print_indent() {
    for (int i = 0; i < m_indent_level; ++i) {
        m_output << "  ";
    }
}

// CompilerFrontend implementation
std::vector<Token> CompilerFrontend::tokenize(const std::string& source, const std::string& filename) {
    Lexer lexer(source, filename);
    auto tokens = lexer.tokenize();
    collect_errors(lexer.errors());
    return tokens;
}

std::unique_ptr<Program> CompilerFrontend::parse(const std::vector<Token>& tokens) {
    Parser parser(tokens);
    auto program = parser.parse();
    collect_errors(parser.errors());
    return program;
}

bool CompilerFrontend::analyze([[maybe_unused]] Program& program) {
    // Basic semantic analysis - symbol table population
    // This is a simplified implementation
    m_symbol_table.push_scope(); // Global scope
    
    // In a full implementation, this would involve:
    // 1. Type checking
    // 2. Symbol resolution
    // 3. Control flow analysis
    // 4. Dead code detection
    // etc.
    
    return m_errors.empty();
}

void CompilerFrontend::collect_errors(const std::vector<CompilerError>& errors) {
    m_errors.insert(m_errors.end(), errors.begin(), errors.end());
}

} // namespace compiler_frontend
