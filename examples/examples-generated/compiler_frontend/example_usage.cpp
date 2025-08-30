// MIT License
// Copyright (c) 2025 dbjwhs

#include "compiler_frontend.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <map>

using namespace compiler_frontend;

/**
 * @brief Example usage of the compiler frontend
 * 
 * This demonstrates how to use the compiler frontend library to:
 * - Tokenize source code
 * - Parse tokens into an AST
 * - Handle errors gracefully
 * - Traverse the AST using the visitor pattern
 * - Perform basic semantic analysis
 */

// Custom visitor for counting different node types
class NodeCountVisitor : public ASTVisitor {
public:
    struct Counts {
        int functions = 0;
        int variables = 0;
        int expressions = 0;
        int statements = 0;
        int literals = 0;
    };

    void visit_binary_expr(BinaryExpr& node) override {
        ++m_counts.expressions;
        const_cast<Expression&>(node.left()).accept(*this);
        const_cast<Expression&>(node.right()).accept(*this);
    }

    void visit_unary_expr(UnaryExpr& node) override {
        ++m_counts.expressions;
        const_cast<Expression&>(node.operand()).accept(*this);
    }

    void visit_literal_expr([[maybe_unused]] LiteralExpr& node) override {
        ++m_counts.expressions;
        ++m_counts.literals;
    }

    void visit_identifier_expr([[maybe_unused]] IdentifierExpr& node) override {
        ++m_counts.expressions;
    }

    void visit_call_expr(CallExpr& node) override {
        ++m_counts.expressions;
        const_cast<Expression&>(node.callee()).accept(*this);
        for (const auto& arg : node.arguments()) {
            arg->accept(*this);
        }
    }

    void visit_expression_stmt(ExpressionStmt& node) override {
        ++m_counts.statements;
        const_cast<Expression&>(node.expression()).accept(*this);
    }

    void visit_variable_decl(VariableDecl& node) override {
        ++m_counts.statements;
        ++m_counts.variables;
        if (node.initializer()) {
            node.initializer()->accept(*this);
        }
    }

    void visit_function_decl(FunctionDecl& node) override {
        ++m_counts.statements;
        ++m_counts.functions;
        const_cast<Statement&>(node.body()).accept(*this);
    }

    void visit_if_stmt(IfStmt& node) override {
        ++m_counts.statements;
        const_cast<Expression&>(node.condition()).accept(*this);
        const_cast<Statement&>(node.then_stmt()).accept(*this);
        if (node.else_stmt()) {
            node.else_stmt()->accept(*this);
        }
    }

    void visit_while_stmt(WhileStmt& node) override {
        ++m_counts.statements;
        const_cast<Expression&>(node.condition()).accept(*this);
        const_cast<Statement&>(node.body()).accept(*this);
    }

    void visit_return_stmt(ReturnStmt& node) override {
        ++m_counts.statements;
        if (node.value()) {
            node.value()->accept(*this);
        }
    }

    void visit_block_stmt(BlockStmt& node) override {
        ++m_counts.statements;
        for (const auto& stmt : node.statements()) {
            stmt->accept(*this);
        }
    }

    void visit_program(Program& node) override {
        for (const auto& stmt : node.statements()) {
            stmt->accept(*this);
        }
    }

    const Counts& counts() const { return m_counts; }

private:
    Counts m_counts;
};

void demonstrate_basic_usage() {
    std::cout << "\n=== Basic Usage Example ===\n";
    
    // Example source code from the specification
    std::string source = R"(
        function factorial(n: int) -> int {
            if (n <= 1) {
                return 1;
            }
            return n * factorial(n - 1);
        }
        
        function main() -> int {
            var result: int = factorial(5);
            return result;
        }
    )";

    std::cout << "Source code:\n" << source << "\n";

    // Create compiler frontend
    CompilerFrontend compiler;

    // Step 1: Tokenize
    std::cout << "\n1. Tokenizing...\n";
    auto tokens = compiler.tokenize(source);
    std::cout << "   Generated " << tokens.size() << " tokens\n";

    // Step 2: Parse
    std::cout << "\n2. Parsing...\n";
    auto ast = compiler.parse(tokens);
    auto errors = compiler.getErrors();

    if (errors.empty()) {
        std::cout << "   Parsing successful!\n";
        std::cout << "   Generated AST with " << ast->statements().size() << " top-level statements\n";

        // Step 3: Pretty print the AST
        std::cout << "\n3. Pretty-printed AST:\n";
        PrettyPrintVisitor printer;
        ast->accept(printer);

        // Step 4: Count nodes
        std::cout << "\n4. AST Statistics:\n";
        NodeCountVisitor counter;
        ast->accept(counter);
        auto counts = counter.counts();
        
        std::cout << "   Functions: " << counts.functions << "\n";
        std::cout << "   Variables: " << counts.variables << "\n";
        std::cout << "   Expressions: " << counts.expressions << "\n";
        std::cout << "   Statements: " << counts.statements << "\n";
        std::cout << "   Literals: " << counts.literals << "\n";

    } else {
        std::cout << "   Parsing failed with " << errors.size() << " error(s):\n";
        for (const auto& error : errors) {
            std::cout << "   " << error.format() << "\n";
        }
    }
}

void demonstrate_error_handling() {
    std::cout << "\n=== Error Handling Example ===\n";
    
    // Example with syntax errors
    std::string source_with_errors = R"(
        function broken(x: int) -> int {
            var y = ;  // Missing expression
            return x + y
        }  // Missing semicolon
        
        function another() -> {  // Missing return type
            return "hello";
        }
    )";

    std::cout << "Source with errors:\n" << source_with_errors << "\n";

    CompilerFrontend compiler;
    auto tokens = compiler.tokenize(source_with_errors);
    auto ast = compiler.parse(tokens);
    auto errors = compiler.getErrors();

    std::cout << "Found " << errors.size() << " error(s):\n";
    for (const auto& error : errors) {
        std::cout << "  " << error.format() << "\n";
    }

    // Even with errors, we can still get a partial AST
    if (ast) {
        std::cout << "\nPartial AST generated with " << ast->statements().size() << " statements\n";
    }
}

void demonstrate_advanced_features() {
    std::cout << "\n=== Advanced Features Example ===\n";

    // Example with complex expressions and operators
    std::string advanced_source = R"(
        function complex_math(a: int, b: int, c: int) -> bool {
            var result: bool = (a + b * c > 10) && !(a < 0 || b < 0);
            return result;
        }
        
        function string_processing(text: string) -> string {
            if (text == "") {
                return "empty";
            }
            return text;
        }
        
        function loops_example() -> int {
            var sum: int = 0;
            var i: int = 0;
            while (i < 10) {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
    )";

    std::cout << "Advanced source code:\n" << advanced_source << "\n";

    CompilerFrontend compiler;
    auto tokens = compiler.tokenize(advanced_source);
    auto ast = compiler.parse(tokens);
    auto errors = compiler.getErrors();

    if (errors.empty()) {
        std::cout << "Parsing successful!\n\n";
        
        // Analyze the AST
        NodeCountVisitor counter;
        ast->accept(counter);
        auto counts = counter.counts();
        
        std::cout << "Code complexity metrics:\n";
        std::cout << "  Functions defined: " << counts.functions << "\n";
        std::cout << "  Variable declarations: " << counts.variables << "\n";
        std::cout << "  Total expressions: " << counts.expressions << "\n";
        std::cout << "  Total statements: " << counts.statements << "\n";
        std::cout << "  Literal values: " << counts.literals << "\n";
        
        // Show some token statistics
        std::map<TokenType, int> token_counts;
        for (const auto& token : tokens) {
            token_counts[token.type]++;
        }
        
        std::cout << "\nToken statistics:\n";
        std::cout << "  Total tokens: " << tokens.size() << "\n";
        std::cout << "  Identifiers: " << token_counts[TokenType::IDENTIFIER] << "\n";
        std::cout << "  Keywords: " << token_counts[TokenType::FUNCTION] + 
                                      token_counts[TokenType::IF] + 
                                      token_counts[TokenType::WHILE] + 
                                      token_counts[TokenType::RETURN] +
                                      token_counts[TokenType::VAR] << "\n";
        std::cout << "  Operators: " << token_counts[TokenType::PLUS] + 
                                       token_counts[TokenType::MINUS] + 
                                       token_counts[TokenType::MULTIPLY] + 
                                       token_counts[TokenType::EQUAL] << "\n";
        
    } else {
        std::cout << "Parsing failed:\n";
        for (const auto& error : errors) {
            std::cout << "  " << error.format() << "\n";
        }
    }
}

void demonstrate_performance() {
    std::cout << "\n=== Performance Example ===\n";
    
    // Generate a large source file
    std::ostringstream large_source;
    large_source << "function generated_code() -> int {\n";
    
    const int num_vars = 500;
    for (int i = 0; i < num_vars; ++i) {
        large_source << "    var x" << i << ": int = " << (i * 2) << " + " << (i + 1) << ";\n";
    }
    
    large_source << "    return x" << (num_vars - 1) << ";\n";
    large_source << "}\n";
    
    std::string source = large_source.str();
    std::cout << "Generated source with " << num_vars << " variable declarations\n";
    std::cout << "Source size: " << source.length() << " characters\n";

    // Time the compilation
    auto start = std::chrono::high_resolution_clock::now();
    
    CompilerFrontend compiler;
    auto tokens = compiler.tokenize(source);
    auto ast = compiler.parse(tokens);
    auto errors = compiler.getErrors();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Compilation completed in " << duration.count() << " ms\n";
    std::cout << "Tokens generated: " << tokens.size() << "\n";
    std::cout << "Errors: " << errors.size() << "\n";
    
    if (errors.empty()) {
        std::cout << "Successfully parsed large source file!\n";
        
        NodeCountVisitor counter;
        ast->accept(counter);
        auto counts = counter.counts();
        
        std::cout << "Final AST contains:\n";
        std::cout << "  " << counts.statements << " statements\n";
        std::cout << "  " << counts.expressions << " expressions\n";
        std::cout << "  " << counts.variables << " variables\n";
    }
}

int main() {
    std::cout << "Compiler Frontend Library - Example Usage\n";
    std::cout << std::string(50, '=') << "\n";
    
    try {
        demonstrate_basic_usage();
        demonstrate_error_handling();
        demonstrate_advanced_features();
        demonstrate_performance();
        
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "All examples completed successfully!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Example failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
