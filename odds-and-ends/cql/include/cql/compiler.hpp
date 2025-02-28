// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_COMPILER_HPP
#define CQL_COMPILER_HPP

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include "visitor.hpp"

namespace cql {

/**
 * Compiler class that transforms AST nodes into formatted query text
 * Implements the QueryVisitor interface to traverse the AST
 */
class QueryCompiler final : public QueryVisitor {
public:
    // Visitor interface implementation
    void visit(const CodeRequestNode& node) override;
    void visit(const ContextNode& node) override;
    void visit(const TestNode& node) override;
    void visit(const DependencyNode& node) override;
    void visit(const PerformanceNode& node) override;
    void visit(const CopyrightNode& node) override;
    
    // Phase 2 directive support
    void visit(const ArchitectureNode& node) override;
    void visit(const ConstraintNode& node) override;
    void visit(const ExampleNode& node) override;
    void visit(const SecurityNode& node) override;
    void visit(const ComplexityNode& node) override;
    void visit(const ModelNode& node) override;
    void visit(const FormatNode& node) override;
    void visit(const VariableNode& node) override;

    // Get the final compiled query string
    [[nodiscard]] std::string get_compiled_query() const;
    
    // Print the compiled query to a stream
    void print_compiled_query(std::ostream& out = std::cout) const;

private:
    // Output content
    std::map<std::string, std::string> m_result_sections;
    std::vector<std::string> m_test_cases;
    std::vector<std::pair<std::string, std::string>> m_examples;
    
    // Output configuration
    std::string m_target_model = "claude-3-opus";
    std::string m_output_format = "markdown";
    
    // Template variables for string interpolation
    std::map<std::string, std::string> m_variables;
    
    // Process string with variable interpolation
    std::string interpolate_variables(const std::string& input) const;
};

} // namespace cql

#endif // CQL_COMPILER_HPP
