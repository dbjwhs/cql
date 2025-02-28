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
 * compiler class that transforms ast nodes into formatted query text
 * implements the queryvisitor interface to traverse the ast
 */
class QueryCompiler final : public QueryVisitor {
public:
    // visitor interface implementation
    void visit(const CodeRequestNode& node) override;
    void visit(const ContextNode& node) override;
    void visit(const TestNode& node) override;
    void visit(const DependencyNode& node) override;
    void visit(const PerformanceNode& node) override;
    void visit(const CopyrightNode& node) override;
    
    // phase 2 directive support
    void visit(const ArchitectureNode& node) override;
    void visit(const ConstraintNode& node) override;
    void visit(const ExampleNode& node) override;
    void visit(const SecurityNode& node) override;
    void visit(const ComplexityNode& node) override;
    void visit(const ModelNode& node) override;
    void visit(const FormatNode& node) override;
    void visit(const VariableNode& node) override;

    // get the final compiled query string
    [[nodiscard]] std::string get_compiled_query() const;
    
    // print the compiled query to a stream
    void print_compiled_query(std::ostream& out = std::cout) const;

private:
    // output content
    std::map<std::string, std::string> m_result_sections;
    std::vector<std::string> m_test_cases;
    std::vector<std::pair<std::string, std::string>> m_examples;
    
    // output configuration
    std::string m_target_model = "claude-3-opus";
    std::string m_output_format = "markdown";
    
    // template variables for string interpolation
    std::map<std::string, std::string> m_variables;
    
    // process string with variable interpolation
    std::string interpolate_variables(const std::string& input) const;
};

} // namespace cql

#endif // cql_compiler_hpp
