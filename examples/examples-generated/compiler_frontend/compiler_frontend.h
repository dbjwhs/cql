// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <concepts>
#include <ranges>
#include <variant>
#include <optional>
#include <source_location>
#include <format>
#include <iostream>
#include <tuple>

namespace compiler_frontend {

// Forward declarations
class ASTNode;
class ASTVisitor;
class CompilerError;

/**
 * @brief Source location information for tokens and AST nodes
 */
struct SourceLocation {
    size_t line = 1;
    size_t column = 1;
    size_t offset = 0;
    std::string filename;

    [[nodiscard]] std::string to_string() const {
        return std::format("{}:{}:{}", filename.empty() ? "<input>" : filename, line, column);
    }
};

/**
 * @brief Token types for lexical analysis
 */
enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    BOOLEAN,
    IDENTIFIER,
    
    // Keywords
    FUNCTION,
    IF,
    ELSE,
    WHILE,
    FOR,
    RETURN,
    VAR,
    CONST,
    TRUE,
    FALSE,
    
    // Types
    INT,
    FLOAT_TYPE,
    STRING_TYPE,
    BOOL_TYPE,
    
    // Operators
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    ASSIGN,
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
    
    // Punctuation
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    COMMA,
    SEMICOLON,
    COLON,
    ARROW,
    
    // Special
    NEWLINE,
    EOF_TOKEN,
    INVALID
};

/**
 * @brief Token representing a lexical unit
 */
struct Token {
    TokenType type;
    std::string value;
    SourceLocation location;

    Token(TokenType t, std::string v, SourceLocation loc) 
        : type(t), value(std::move(v)), location(std::move(loc)) {}

    [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Compiler error with rich diagnostics
 */
class CompilerError {
public:
    enum class Level {
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    CompilerError(Level lvl, std::string msg, SourceLocation loc)
        : m_level(lvl), m_message(std::move(msg)), m_location(std::move(loc)) {}

    [[nodiscard]] Level level() const noexcept { return m_level; }
    [[nodiscard]] const std::string& message() const noexcept { return m_message; }
    [[nodiscard]] const SourceLocation& location() const noexcept { return m_location; }

    [[nodiscard]] std::string format() const;

private:
    Level m_level;
    std::string m_message;
    SourceLocation m_location;
};

/**
 * @brief Base class for all AST nodes with visitor pattern support
 */
class ASTNode {
public:
    explicit ASTNode(SourceLocation loc) : m_location(std::move(loc)) {}
    virtual ~ASTNode() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
    [[nodiscard]] const SourceLocation& location() const noexcept { return m_location; }

protected:
    SourceLocation m_location;
};

/**
 * @brief Visitor interface for AST traversal
 */
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Expression visitors
    virtual void visit_binary_expr(class BinaryExpr& node) = 0;
    virtual void visit_unary_expr(class UnaryExpr& node) = 0;
    virtual void visit_literal_expr(class LiteralExpr& node) = 0;
    virtual void visit_identifier_expr(class IdentifierExpr& node) = 0;
    virtual void visit_call_expr(class CallExpr& node) = 0;
    
    // Statement visitors
    virtual void visit_expression_stmt(class ExpressionStmt& node) = 0;
    virtual void visit_variable_decl(class VariableDecl& node) = 0;
    virtual void visit_function_decl(class FunctionDecl& node) = 0;
    virtual void visit_if_stmt(class IfStmt& node) = 0;
    virtual void visit_while_stmt(class WhileStmt& node) = 0;
    virtual void visit_return_stmt(class ReturnStmt& node) = 0;
    virtual void visit_block_stmt(class BlockStmt& node) = 0;
    virtual void visit_program(class Program& node) = 0;
};


// AST Node Implementations

/**
 * @brief Base class for expressions
 */
class Expression : public ASTNode {
public:
    explicit Expression(SourceLocation loc) : ASTNode(std::move(loc)) {}
};

/**
 * @brief Binary expression node (e.g., a + b, x == y)
 */
class BinaryExpr : public Expression {
public:
    BinaryExpr(std::unique_ptr<Expression> left, Token op, std::unique_ptr<Expression> right)
        : Expression(left->location()), m_left(std::move(left)), m_operator(std::move(op)), m_right(std::move(right)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_binary_expr(*this); }

    [[nodiscard]] const Expression& left() const noexcept { return *m_left; }
    [[nodiscard]] const Token& operator_token() const noexcept { return m_operator; }
    [[nodiscard]] const Expression& right() const noexcept { return *m_right; }

private:
    std::unique_ptr<Expression> m_left;
    Token m_operator;
    std::unique_ptr<Expression> m_right;
};

/**
 * @brief Unary expression node (e.g., -x, !flag)
 */
class UnaryExpr : public Expression {
public:
    UnaryExpr(Token op, std::unique_ptr<Expression> operand)
        : Expression(op.location), m_operator(std::move(op)), m_operand(std::move(operand)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_unary_expr(*this); }

    [[nodiscard]] const Token& operator_token() const noexcept { return m_operator; }
    [[nodiscard]] const Expression& operand() const noexcept { return *m_operand; }

private:
    Token m_operator;
    std::unique_ptr<Expression> m_operand;
};

/**
 * @brief Literal expression node (numbers, strings, booleans)
 */
class LiteralExpr : public Expression {
public:
    using Value = std::variant<int64_t, double, std::string, bool>;

    LiteralExpr(Value val, SourceLocation loc)
        : Expression(std::move(loc)), m_value(std::move(val)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_literal_expr(*this); }

    [[nodiscard]] const Value& value() const noexcept { return m_value; }

private:
    Value m_value;
};

/**
 * @brief Identifier expression node
 */
class IdentifierExpr : public Expression {
public:
    IdentifierExpr(std::string name, SourceLocation loc)
        : Expression(std::move(loc)), m_name(std::move(name)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_identifier_expr(*this); }

    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

private:
    std::string m_name;
};

/**
 * @brief Function call expression node
 */
class CallExpr : public Expression {
public:
    CallExpr(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> args)
        : Expression(callee->location()), m_callee(std::move(callee)), m_arguments(std::move(args)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_call_expr(*this); }

    [[nodiscard]] const Expression& callee() const noexcept { return *m_callee; }
    [[nodiscard]] const std::vector<std::unique_ptr<Expression>>& arguments() const noexcept { return m_arguments; }

private:
    std::unique_ptr<Expression> m_callee;
    std::vector<std::unique_ptr<Expression>> m_arguments;
};

/**
 * @brief Base class for statements
 */
class Statement : public ASTNode {
public:
    explicit Statement(SourceLocation loc) : ASTNode(std::move(loc)) {}
};

/**
 * @brief Expression statement node
 */
class ExpressionStmt : public Statement {
public:
    explicit ExpressionStmt(std::unique_ptr<Expression> expr)
        : Statement(expr->location()), m_expression(std::move(expr)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_expression_stmt(*this); }

    [[nodiscard]] const Expression& expression() const noexcept { return *m_expression; }

private:
    std::unique_ptr<Expression> m_expression;
};

/**
 * @brief Variable declaration node
 */
class VariableDecl : public Statement {
public:
    VariableDecl(std::string name, std::optional<std::string> type, std::unique_ptr<Expression> init, SourceLocation loc)
        : Statement(std::move(loc)), m_name(std::move(name)), m_type(std::move(type)), m_initializer(std::move(init)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_variable_decl(*this); }

    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] const std::optional<std::string>& type() const noexcept { return m_type; }
    [[nodiscard]] const std::unique_ptr<Expression>& initializer() const noexcept { return m_initializer; }

private:
    std::string m_name;
    std::optional<std::string> m_type;
    std::unique_ptr<Expression> m_initializer;
};

/**
 * @brief Function parameter
 */
struct Parameter {
    std::string name;
    std::string type;
    SourceLocation location;
};

/**
 * @brief Function declaration node
 */
class FunctionDecl : public Statement {
public:
    FunctionDecl(std::string name, std::vector<Parameter> params, std::string return_type, 
                 std::unique_ptr<Statement> body, SourceLocation loc)
        : Statement(std::move(loc)), m_name(std::move(name)), m_parameters(std::move(params)),
          m_return_type(std::move(return_type)), m_body(std::move(body)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_function_decl(*this); }

    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] const std::vector<Parameter>& parameters() const noexcept { return m_parameters; }
    [[nodiscard]] const std::string& return_type() const noexcept { return m_return_type; }
    [[nodiscard]] const Statement& body() const noexcept { return *m_body; }

private:
    std::string m_name;
    std::vector<Parameter> m_parameters;
    std::string m_return_type;
    std::unique_ptr<Statement> m_body;
};

/**
 * @brief If statement node
 */
class IfStmt : public Statement {
public:
    IfStmt(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> then_stmt, 
           std::unique_ptr<Statement> else_stmt = nullptr)
        : Statement(condition->location()), m_condition(std::move(condition)),
          m_then_stmt(std::move(then_stmt)), m_else_stmt(std::move(else_stmt)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_if_stmt(*this); }

    [[nodiscard]] const Expression& condition() const noexcept { return *m_condition; }
    [[nodiscard]] const Statement& then_stmt() const noexcept { return *m_then_stmt; }
    [[nodiscard]] const std::unique_ptr<Statement>& else_stmt() const noexcept { return m_else_stmt; }

private:
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<Statement> m_then_stmt;
    std::unique_ptr<Statement> m_else_stmt;
};

/**
 * @brief While statement node
 */
class WhileStmt : public Statement {
public:
    WhileStmt(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
        : Statement(condition->location()), m_condition(std::move(condition)), m_body(std::move(body)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_while_stmt(*this); }

    [[nodiscard]] const Expression& condition() const noexcept { return *m_condition; }
    [[nodiscard]] const Statement& body() const noexcept { return *m_body; }

private:
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<Statement> m_body;
};

/**
 * @brief Return statement node
 */
class ReturnStmt : public Statement {
public:
    ReturnStmt(std::unique_ptr<Expression> value, SourceLocation loc)
        : Statement(std::move(loc)), m_value(std::move(value)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_return_stmt(*this); }

    [[nodiscard]] const std::unique_ptr<Expression>& value() const noexcept { return m_value; }

private:
    std::unique_ptr<Expression> m_value;
};

/**
 * @brief Block statement node
 */
class BlockStmt : public Statement {
public:
    BlockStmt(std::vector<std::unique_ptr<Statement>> statements, SourceLocation loc)
        : Statement(std::move(loc)), m_statements(std::move(statements)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_block_stmt(*this); }

    [[nodiscard]] const std::vector<std::unique_ptr<Statement>>& statements() const noexcept { return m_statements; }

private:
    std::vector<std::unique_ptr<Statement>> m_statements;
};

/**
 * @brief Root program node
 */
class Program : public ASTNode {
public:
    Program(std::vector<std::unique_ptr<Statement>> statements, SourceLocation loc)
        : ASTNode(std::move(loc)), m_statements(std::move(statements)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit_program(*this); }

    [[nodiscard]] const std::vector<std::unique_ptr<Statement>>& statements() const noexcept { return m_statements; }

private:
    std::vector<std::unique_ptr<Statement>> m_statements;
};

/**
 * @brief Symbol information for symbol table
 */
struct Symbol {
    enum class Type {
        VARIABLE,
        FUNCTION,
        PARAMETER
    };

    std::string name;
    std::string data_type;
    Type symbol_type;
    SourceLocation declaration_location;
    bool is_mutable = true;
};

/**
 * @brief Scoped symbol table with hash-based storage
 */
class SymbolTable {
public:
    SymbolTable() { push_scope(); }

    void push_scope();
    void pop_scope();
    
    bool declare(const Symbol& symbol);
    [[nodiscard]] std::optional<Symbol> lookup(const std::string& name) const;
    [[nodiscard]] bool is_declared_in_current_scope(const std::string& name) const;
    
    [[nodiscard]] size_t scope_depth() const noexcept { return m_scopes.size(); }

private:
    using ScopeMap = std::unordered_map<std::string, Symbol>;
    std::vector<ScopeMap> m_scopes;
};

/**
 * @brief Lexer for tokenizing source code
 */
class Lexer {
public:
    explicit Lexer(std::string source, std::string filename = "");

    [[nodiscard]] std::vector<Token> tokenize();
    [[nodiscard]] const std::vector<CompilerError>& errors() const noexcept { return m_errors; }

private:
    struct TokenRule {
        TokenType type;
        std::regex pattern;
        int priority;
    };

    std::string m_source;
    std::string m_filename;
    size_t m_position = 0;
    size_t m_line = 1;
    size_t m_column = 1;
    std::vector<CompilerError> m_errors;
    std::vector<TokenRule> m_token_rules;

    void initialize_token_rules();
    [[nodiscard]] std::optional<Token> next_token();
    [[nodiscard]] SourceLocation current_location() const;
    void advance(size_t count = 1);
    void skip_whitespace();
    void skip_comment();
    void add_error(CompilerError::Level level, const std::string& message);
};

/**
 * @brief Parser for building AST from tokens
 */
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    [[nodiscard]] std::unique_ptr<Program> parse();
    [[nodiscard]] const std::vector<CompilerError>& errors() const noexcept { return m_errors; }

private:
    std::vector<Token> m_tokens;
    size_t m_current = 0;
    std::vector<CompilerError> m_errors;

    // Parsing methods
    [[nodiscard]] std::unique_ptr<Statement> parse_statement();
    [[nodiscard]] std::unique_ptr<Statement> parse_function_declaration();
    [[nodiscard]] std::unique_ptr<Statement> parse_variable_declaration();
    [[nodiscard]] std::unique_ptr<Statement> parse_if_statement();
    [[nodiscard]] std::unique_ptr<Statement> parse_while_statement();
    [[nodiscard]] std::unique_ptr<Statement> parse_return_statement();
    [[nodiscard]] std::unique_ptr<Statement> parse_block_statement();
    [[nodiscard]] std::unique_ptr<Statement> parse_expression_statement();
    
    [[nodiscard]] std::unique_ptr<Expression> parse_expression();
    [[nodiscard]] std::unique_ptr<Expression> parse_logical_or();
    [[nodiscard]] std::unique_ptr<Expression> parse_logical_and();
    [[nodiscard]] std::unique_ptr<Expression> parse_equality();
    [[nodiscard]] std::unique_ptr<Expression> parse_comparison();
    [[nodiscard]] std::unique_ptr<Expression> parse_term();
    [[nodiscard]] std::unique_ptr<Expression> parse_factor();
    [[nodiscard]] std::unique_ptr<Expression> parse_unary();
    [[nodiscard]] std::unique_ptr<Expression> parse_call();
    [[nodiscard]] std::unique_ptr<Expression> parse_primary();

    // Utility methods
    [[nodiscard]] bool match(std::initializer_list<TokenType> types);
    [[nodiscard]] bool check(TokenType type) const;
    [[nodiscard]] const Token& advance();
    [[nodiscard]] bool is_at_end() const;
    [[nodiscard]] const Token& peek() const;
    [[nodiscard]] const Token& previous() const;
    [[nodiscard]] Token consume(TokenType type, const std::string& message);

    void synchronize();
    void add_error(CompilerError::Level level, const std::string& message);
    [[nodiscard]] SourceLocation current_location() const;
};

/**
 * @brief Pretty print visitor for AST visualization
 */
class PrettyPrintVisitor : public ASTVisitor {
public:
    explicit PrettyPrintVisitor(std::ostream& output) : m_output(output) {}
    PrettyPrintVisitor() : m_output(std::cout) {}

    void visit_binary_expr(BinaryExpr& node) override;
    void visit_unary_expr(UnaryExpr& node) override;
    void visit_literal_expr(LiteralExpr& node) override;
    void visit_identifier_expr(IdentifierExpr& node) override;
    void visit_call_expr(CallExpr& node) override;
    void visit_expression_stmt(ExpressionStmt& node) override;
    void visit_variable_decl(VariableDecl& node) override;
    void visit_function_decl(FunctionDecl& node) override;
    void visit_if_stmt(IfStmt& node) override;
    void visit_while_stmt(WhileStmt& node) override;
    void visit_return_stmt(ReturnStmt& node) override;
    void visit_block_stmt(BlockStmt& node) override;
    void visit_program(Program& node) override;

private:
    std::ostream& m_output;
    int m_indent_level = 0;

    void print_indent();
    void increase_indent() { ++m_indent_level; }
    void decrease_indent() { --m_indent_level; }
};

/**
 * @brief Main compiler frontend interface
 */
class CompilerFrontend {
public:
    CompilerFrontend() = default;

    [[nodiscard]] std::vector<Token> tokenize(const std::string& source, const std::string& filename = "");
    [[nodiscard]] std::unique_ptr<Program> parse(const std::vector<Token>& tokens);
    
    [[nodiscard]] const std::vector<CompilerError>& getErrors() const noexcept { return m_errors; }
    void clearErrors() { m_errors.clear(); }

    // Semantic analysis
    bool analyze(Program& program);
    [[nodiscard]] const SymbolTable& getSymbolTable() const noexcept { return m_symbol_table; }

private:
    std::vector<CompilerError> m_errors;
    SymbolTable m_symbol_table;

    void collect_errors(const std::vector<CompilerError>& errors);
};

} // namespace compiler_frontend
