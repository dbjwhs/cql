// MIT License
// Copyright (c) 2025 dbjwhs

#include "compiler_frontend.h"
#include <cassert>
#include <iostream>
#include <sstream>

using namespace compiler_frontend;

class TestRunner {
public:
    static void run_all_tests() {
        std::cout << "Running Compiler Frontend Tests...\n\n";
        
        test_tokenization();
        test_parsing_precedence();
        test_error_recovery();
        test_visitor_pattern();
        test_symbol_table();
        test_integration();
        
        std::cout << "\nAll tests passed! ✅\n";
    }

private:
    static void test_tokenization() {
        std::cout << "Testing tokenization...\n";
        
        // Test 1: Basic tokens
        {
            std::string source = "function main() -> int { return 42; }";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            
            assert(tokens.size() == 12); // Including semicolon and EOF
            assert(tokens[0].type == TokenType::FUNCTION);
            assert(tokens[1].type == TokenType::IDENTIFIER);
            assert(tokens[1].value == "main");
            assert(tokens[2].type == TokenType::LEFT_PAREN);
            assert(tokens[3].type == TokenType::RIGHT_PAREN);
            assert(tokens[4].type == TokenType::ARROW);
            assert(tokens[5].type == TokenType::INT);
            assert(tokens[6].type == TokenType::LEFT_BRACE);
            assert(tokens[7].type == TokenType::RETURN);
            assert(tokens[8].type == TokenType::INTEGER);
            assert(tokens[8].value == "42");
            assert(tokens[9].type == TokenType::SEMICOLON);
            assert(tokens[10].type == TokenType::RIGHT_BRACE);
        }
        
        // Test 2: Operators and precedence
        {
            std::string source = "x + y * z == 10 && !flag";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            
            assert(tokens[0].type == TokenType::IDENTIFIER);
            assert(tokens[1].type == TokenType::PLUS);
            assert(tokens[2].type == TokenType::IDENTIFIER);
            assert(tokens[3].type == TokenType::MULTIPLY);
            assert(tokens[4].type == TokenType::IDENTIFIER);
            assert(tokens[5].type == TokenType::EQUAL);
            assert(tokens[6].type == TokenType::INTEGER);
            assert(tokens[7].type == TokenType::LOGICAL_AND);
            assert(tokens[8].type == TokenType::LOGICAL_NOT);
            assert(tokens[9].type == TokenType::IDENTIFIER);
        }
        
        // Test 3: String literals
        {
            std::string source = R"("Hello, World!" "with \"quotes\"")";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            
            assert(tokens[0].type == TokenType::STRING);
            assert(tokens[0].value == "\"Hello, World!\"");
            assert(tokens[1].type == TokenType::STRING);
            assert(tokens[1].value == "\"with \\\"quotes\\\"\"");
        }
        
        // Test 4: Float literals
        {
            std::string source = "3.14 0.5 123.456";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            
            assert(tokens[0].type == TokenType::FLOAT);
            assert(tokens[0].value == "3.14");
            assert(tokens[1].type == TokenType::FLOAT);
            assert(tokens[1].value == "0.5");
            assert(tokens[2].type == TokenType::FLOAT);
            assert(tokens[2].value == "123.456");
        }
        
        // Test 5: Keywords vs identifiers
        {
            std::string source = "if ifdef function func true truly";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            
            assert(tokens[0].type == TokenType::IF);
            assert(tokens[1].type == TokenType::IDENTIFIER);
            assert(tokens[1].value == "ifdef");
            assert(tokens[2].type == TokenType::FUNCTION);
            assert(tokens[3].type == TokenType::IDENTIFIER);
            assert(tokens[3].value == "func");
            assert(tokens[4].type == TokenType::TRUE);
            assert(tokens[5].type == TokenType::IDENTIFIER);
            assert(tokens[5].value == "truly");
        }
        
        std::cout << "  ✅ Tokenization tests passed\n";
    }
    
    static void test_parsing_precedence() {
        std::cout << "Testing parsing with precedence...\n";
        
        // Test 1: Arithmetic precedence
        {
            std::string source = "a + b * c;";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto program = compiler.parse(tokens);
            
            assert(program != nullptr);
            assert(program->statements().size() == 1);
            
            auto expr_stmt = dynamic_cast<ExpressionStmt*>(program->statements()[0].get());
            assert(expr_stmt != nullptr);
            
            auto binary_expr = dynamic_cast<const BinaryExpr*>(&expr_stmt->expression());
            assert(binary_expr != nullptr);
            assert(binary_expr->operator_token().type == TokenType::PLUS);
            
            // Right side should be b * c
            auto right_binary = dynamic_cast<const BinaryExpr*>(&binary_expr->right());
            assert(right_binary != nullptr);
            assert(right_binary->operator_token().type == TokenType::MULTIPLY);
        }
        
        // Test 2: Comparison and logical operators
        {
            std::string source = "a < b && c == d || e > f;";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto program = compiler.parse(tokens);
            
            assert(program != nullptr);
            assert(compiler.getErrors().empty());
        }
        
        // Test 3: Function calls
        {
            std::string source = "factorial(n - 1);";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto program = compiler.parse(tokens);
            
            assert(program != nullptr);
            assert(program->statements().size() == 1);
            
            auto expr_stmt = dynamic_cast<ExpressionStmt*>(program->statements()[0].get());
            assert(expr_stmt != nullptr);
            
            auto call_expr = dynamic_cast<const CallExpr*>(&expr_stmt->expression());
            assert(call_expr != nullptr);
            assert(call_expr->arguments().size() == 1);
        }
        
        // Test 4: Parenthesized expressions
        {
            std::string source = "(a + b) * c;";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto program = compiler.parse(tokens);
            
            assert(program != nullptr);
            assert(compiler.getErrors().empty());
            
            auto expr_stmt = dynamic_cast<ExpressionStmt*>(program->statements()[0].get());
            assert(expr_stmt != nullptr);
            
            auto binary_expr = dynamic_cast<const BinaryExpr*>(&expr_stmt->expression());
            assert(binary_expr != nullptr);
            assert(binary_expr->operator_token().type == TokenType::MULTIPLY);
            
            // Left side should be (a + b)
            auto left_binary = dynamic_cast<const BinaryExpr*>(&binary_expr->left());
            assert(left_binary != nullptr);
            assert(left_binary->operator_token().type == TokenType::PLUS);
        }
        
        std::cout << "  ✅ Parsing precedence tests passed\n";
    }
    
    static void test_error_recovery() {
        std::cout << "Testing error recovery...\n";
        
        // Test 1: Missing semicolon
        {
            std::string source = "var x = 10 var y = 20;";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto program = compiler.parse(tokens);
            
            assert(!compiler.getErrors().empty());
            // Parser should recover and parse the second variable declaration
        }
        
        // Test 2: Invalid token
        {
            std::string source = "var x = @invalid;";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            
            assert(!compiler.getErrors().empty());
            auto& errors = compiler.getErrors();
            bool found_invalid_char = false;
            for (const auto& error : errors) {
                if (error.message().find("Unexpected character") != std::string::npos) {
                    found_invalid_char = true;
                    break;
                }
            }
            assert(found_invalid_char);
        }
        
        // Test 3: Unmatched parentheses
        {
            std::string source = "function test() { if (x > 0 { return 1; } }";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto program = compiler.parse(tokens);
            
            assert(!compiler.getErrors().empty());
        }
        
        // Test 4: Missing function body
        {
            std::string source = "function test();";
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto program = compiler.parse(tokens);
            
            assert(!compiler.getErrors().empty());
        }
        
        std::cout << "  ✅ Error recovery tests passed\n";
    }
    
    static void test_visitor_pattern() {
        std::cout << "Testing visitor pattern...\n";
        
        // Test AST traversal with PrettyPrintVisitor
        {
            std::string source = R"(
                function factorial(n: int) -> int {
                    if (n <= 1) {
                        return 1;
                    }
                    return n * factorial(n - 1);
                }
            )";
            
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto program = compiler.parse(tokens);
            
            assert(program != nullptr);
            assert(compiler.getErrors().empty());
            
            // Test PrettyPrintVisitor
            std::ostringstream output;
            PrettyPrintVisitor printer(output);
            program->accept(printer);
            
            std::string result = output.str();
            assert(result.find("function factorial") != std::string::npos);
            assert(result.find("if (") != std::string::npos);
            assert(result.find("return") != std::string::npos);
        }
        
        // Test individual node types
        {
            // Create a simple binary expression: 2 + 3
            auto left = std::make_unique<LiteralExpr>(int64_t(2), SourceLocation{});
            Token op(TokenType::PLUS, "+", SourceLocation{});
            auto right = std::make_unique<LiteralExpr>(int64_t(3), SourceLocation{});
            
            auto binary_expr = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
            
            std::ostringstream output;
            PrettyPrintVisitor printer(output);
            binary_expr->accept(printer);
            
            assert(output.str() == "(2 + 3)");
        }
        
        std::cout << "  ✅ Visitor pattern tests passed\n";
    }
    
    static void test_symbol_table() {
        std::cout << "Testing symbol table...\n";
        
        // Test 1: Basic declaration and lookup
        {
            SymbolTable table;
            Symbol symbol{"x", "int", Symbol::Type::VARIABLE, SourceLocation{}};
            assert(table.declare(symbol));
            
            auto found = table.lookup("x");
            assert(found.has_value());
            assert(found->name == "x");
            assert(found->data_type == "int");
            assert(found->symbol_type == Symbol::Type::VARIABLE);
        }
        
        // Test 2: Scope management
        {
            SymbolTable table;
            Symbol outer{"outer", "int", Symbol::Type::VARIABLE, SourceLocation{}};
            assert(table.declare(outer));
            
            table.push_scope();
            
            Symbol inner{"inner", "string", Symbol::Type::VARIABLE, SourceLocation{}};
            assert(table.declare(inner));
            
            // Both should be visible from inner scope
            assert(table.lookup("outer").has_value());
            assert(table.lookup("inner").has_value());
            
            table.pop_scope();
            
            // Only outer should be visible
            assert(table.lookup("outer").has_value());
            assert(!table.lookup("inner").has_value());
        }
        
        // Test 3: Shadowing
        {
            SymbolTable table;
            Symbol outer{"x", "int", Symbol::Type::VARIABLE, SourceLocation{}};
            assert(table.declare(outer));
            
            table.push_scope();
            
            Symbol inner{"x", "string", Symbol::Type::VARIABLE, SourceLocation{}};
            assert(table.declare(inner));
            
            // Inner should shadow outer
            auto found = table.lookup("x");
            assert(found.has_value());
            assert(found->data_type == "string");
            
            table.pop_scope();
            
            // Now outer should be visible again
            found = table.lookup("x");
            assert(found.has_value());
            assert(found->data_type == "int");
        }
        
        // Test 4: Redeclaration in same scope
        {
            SymbolTable table;
            Symbol first{"y", "int", Symbol::Type::VARIABLE, SourceLocation{}};
            assert(table.declare(first));
            
            Symbol second{"y", "string", Symbol::Type::VARIABLE, SourceLocation{}};
            assert(!table.declare(second)); // Should fail - already declared in same scope
        }
        
        // Test 5: Different symbol types
        {
            SymbolTable table;
            Symbol var{"myVar", "int", Symbol::Type::VARIABLE, SourceLocation{}};
            Symbol func{"myFunc", "int -> int", Symbol::Type::FUNCTION, SourceLocation{}};
            Symbol param{"myParam", "string", Symbol::Type::PARAMETER, SourceLocation{}};
            
            assert(table.declare(var));
            assert(table.declare(func));
            assert(table.declare(param));
            
            auto var_found = table.lookup("myVar");
            assert(var_found.has_value());
            assert(var_found->symbol_type == Symbol::Type::VARIABLE);
            
            auto func_found = table.lookup("myFunc");
            assert(func_found.has_value());
            assert(func_found->symbol_type == Symbol::Type::FUNCTION);
            
            auto param_found = table.lookup("myParam");
            assert(param_found.has_value());
            assert(param_found->symbol_type == Symbol::Type::PARAMETER);
        }
        
        std::cout << "  ✅ Symbol table tests passed\n";
    }
    
    static void test_integration() {
        std::cout << "Testing integration...\n";
        
        // Test the complete pipeline with the example from requirements
        {
            std::string source = R"(
                function factorial(n: int) -> int {
                    if (n <= 1) {
                        return 1;
                    }
                    return n * factorial(n - 1);
                }
                
                var result: int = factorial(5);
            )";
            
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto ast = compiler.parse(tokens);
            auto errors = compiler.getErrors();
            
            // Should parse without errors
            assert(errors.empty());
            assert(ast != nullptr);
            
            // Check AST structure
            assert(ast->statements().size() == 2); // function + variable declaration
            
            // First statement should be function declaration
            auto func_decl = dynamic_cast<FunctionDecl*>(ast->statements()[0].get());
            assert(func_decl != nullptr);
            assert(func_decl->name() == "factorial");
            assert(func_decl->parameters().size() == 1);
            assert(func_decl->parameters()[0].name == "n");
            assert(func_decl->parameters()[0].type == "int");
            assert(func_decl->return_type() == "int");
            
            // Second statement should be variable declaration
            auto var_decl = dynamic_cast<VariableDecl*>(ast->statements()[1].get());
            assert(var_decl != nullptr);
            assert(var_decl->name() == "result");
            assert(var_decl->type() == "int");
            assert(var_decl->initializer() != nullptr);
            
            // Test pretty printing
            std::ostringstream output;
            PrettyPrintVisitor printer(output);
            ast->accept(printer);
            
            std::string result = output.str();
            assert(result.find("function factorial") != std::string::npos);
            assert(result.find("var result") != std::string::npos);
        }
        
        // Test error handling in integration
        {
            std::string source = R"(
                function broken(x: int) -> int {
                    return x +;  // Syntax error
                }
                
                var y = undeclared_func();  // Semantic error (would be caught in full analysis)
            )";
            
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(source);
            auto ast = compiler.parse(tokens);
            auto errors = compiler.getErrors();
            
            // Should have parse errors
            assert(!errors.empty());
        }
        
        // Test performance with larger input
        {
            std::ostringstream large_source;
            large_source << "function test() -> int {\n";
            
            // Generate 100 variable declarations (reduced for testing)
            for (int i = 0; i < 100; ++i) {
                large_source << "    var x" << i << ": int = " << i << ";\n";
            }
            
            large_source << "    return x99;\n}\n";
            
            CompilerFrontend compiler;
            auto tokens = compiler.tokenize(large_source.str());
            auto ast = compiler.parse(tokens);
            
            // Should handle large input without issues
            assert(ast != nullptr);
            assert(compiler.getErrors().empty());
        }
        
        std::cout << "  ✅ Integration tests passed\n";
    }
};

// Semantic Analysis Visitor for testing
class SemanticAnalysisVisitor : public ASTVisitor {
public:
    explicit SemanticAnalysisVisitor(SymbolTable& table) : m_symbol_table(table) {}
    
    void visit_binary_expr(BinaryExpr& node) override {
        const_cast<Expression&>(node.left()).accept(*this);
        const_cast<Expression&>(node.right()).accept(*this);
    }
    
    void visit_unary_expr(UnaryExpr& node) override {
        const_cast<Expression&>(node.operand()).accept(*this);
    }
    
    void visit_literal_expr([[maybe_unused]] LiteralExpr& node) override {
        // Nothing to do for literals
    }
    
    void visit_identifier_expr(IdentifierExpr& node) override {
        if (!m_symbol_table.lookup(node.name())) {
            m_errors.emplace_back(CompilerError::Level::ERROR, 
                                 "Undefined identifier: " + node.name(),
                                 node.location());
        }
    }
    
    void visit_call_expr(CallExpr& node) override {
        const_cast<Expression&>(node.callee()).accept(*this);
        for (const auto& arg : node.arguments()) {
            arg->accept(*this);
        }
    }
    
    void visit_expression_stmt(ExpressionStmt& node) override {
        const_cast<Expression&>(node.expression()).accept(*this);
    }
    
    void visit_variable_decl(VariableDecl& node) override {
        if (node.initializer()) {
            node.initializer()->accept(*this);
        }
        
        Symbol symbol{node.name(), node.type().value_or("auto"), 
                     Symbol::Type::VARIABLE, node.location()};
        
        if (!m_symbol_table.declare(symbol)) {
            m_errors.emplace_back(CompilerError::Level::ERROR,
                                 "Variable already declared: " + node.name(),
                                 node.location());
        }
    }
    
    void visit_function_decl(FunctionDecl& node) override {
        // Declare function in current scope
        std::string func_type = "(";
        bool first = true;
        for (const auto& param : node.parameters()) {
            if (!first) func_type += ", ";
            func_type += param.type;
            first = false;
        }
        func_type += ") -> " + node.return_type();
        
        Symbol func_symbol{node.name(), func_type, Symbol::Type::FUNCTION, node.location()};
        if (!m_symbol_table.declare(func_symbol)) {
            m_errors.emplace_back(CompilerError::Level::ERROR,
                                 "Function already declared: " + node.name(),
                                 node.location());
        }
        
        // Create new scope for function body
        m_symbol_table.push_scope();
        
        // Declare parameters
        for (const auto& param : node.parameters()) {
            Symbol param_symbol{param.name, param.type, Symbol::Type::PARAMETER, param.location};
            m_symbol_table.declare(param_symbol);
        }
        
        // Analyze function body
        const_cast<Statement&>(node.body()).accept(*this);
        
        m_symbol_table.pop_scope();
    }
    
    void visit_if_stmt(IfStmt& node) override {
        const_cast<Expression&>(node.condition()).accept(*this);
        const_cast<Statement&>(node.then_stmt()).accept(*this);
        if (node.else_stmt()) {
            node.else_stmt()->accept(*this);
        }
    }
    
    void visit_while_stmt(WhileStmt& node) override {
        const_cast<Expression&>(node.condition()).accept(*this);
        const_cast<Statement&>(node.body()).accept(*this);
    }
    
    void visit_return_stmt(ReturnStmt& node) override {
        if (node.value()) {
            node.value()->accept(*this);
        }
    }
    
    void visit_block_stmt(BlockStmt& node) override {
        m_symbol_table.push_scope();
        
        for (const auto& stmt : node.statements()) {
            stmt->accept(*this);
        }
        
        m_symbol_table.pop_scope();
    }
    
    void visit_program(Program& node) override {
        for (const auto& stmt : node.statements()) {
            stmt->accept(*this);
        }
    }
    
    const std::vector<CompilerError>& errors() const { return m_errors; }

private:
    SymbolTable& m_symbol_table;
    std::vector<CompilerError> m_errors;
};

int main() {
    try {
        TestRunner::run_all_tests();
        
        // Additional demonstration
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "Demonstration of the Compiler Frontend\n";
        std::cout << std::string(50, '=') << "\n\n";
        
        std::string source = R"(
            function fibonacci(n: int) -> int {
                if (n <= 1) {
                    return n;
                }
                return fibonacci(n - 1) + fibonacci(n - 2);
            }
            
            function main() -> int {
                var result: int = fibonacci(10);
                return result;
            }
        )";
        
        std::cout << "Source code:\n" << source << "\n\n";
        
        CompilerFrontend compiler;
        
        // Tokenization
        std::cout << "1. Tokenization:\n";
        auto tokens = compiler.tokenize(source, "demo.lang");
        std::cout << "   Generated " << tokens.size() << " tokens\n";
        
        // Parsing
        std::cout << "\n2. Parsing:\n";
        auto ast = compiler.parse(tokens);
        auto errors = compiler.getErrors();
        
        if (errors.empty()) {
            std::cout << "   Parsing successful!\n";
            std::cout << "   Generated AST with " << ast->statements().size() << " top-level statements\n";
        } else {
            std::cout << "   Parsing errors:\n";
            for (const auto& error : errors) {
                std::cout << "   " << error.format() << "\n";
            }
        }
        
        // Pretty printing
        if (ast && errors.empty()) {
            std::cout << "\n3. Pretty-printed AST:\n";
            PrettyPrintVisitor printer;
            ast->accept(printer);
        }
        
        // Semantic analysis
        std::cout << "\n4. Semantic Analysis:\n";
        SymbolTable symbol_table;
        SemanticAnalysisVisitor analyzer(symbol_table);
        if (ast) {
            ast->accept(analyzer);
            auto semantic_errors = analyzer.errors();
            
            if (semantic_errors.empty()) {
                std::cout << "   No semantic errors found!\n";
                std::cout << "   Symbol table populated with function and variable declarations\n";
            } else {
                std::cout << "   Semantic errors:\n";
                for (const auto& error : semantic_errors) {
                    std::cout << "   " << error.format() << "\n";
                }
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
