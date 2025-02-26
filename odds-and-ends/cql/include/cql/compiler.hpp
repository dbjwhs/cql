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

    // Get the final compiled query string
    [[nodiscard]] std::string get_compiled_query() const;
    
    // Print the compiled query to a stream
    void print_compiled_query(std::ostream& out = std::cout) const;

private:
    std::map<std::string, std::string> m_result_sections;
    std::vector<std::string> m_test_cases;
};

} // namespace cql

#endif // CQL_COMPILER_HPP