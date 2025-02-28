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
 * @class QueryCompiler
 * @brief Compiler that transforms AST nodes into formatted query text
 *
 * The QueryCompiler implements the Visitor pattern to traverse the AST
 * and generate a structured query string suitable for submission to
 * a language model like Claude.
 *
 * The compiler:
 * 1. Visits each node in the AST
 * 2. Extracts relevant information from each node
 * 3. Organizes the information into sections
 * 4. Formats the final query according to the specified output format
 * 5. Handles variable interpolation for template substitution
 *
 * The final query includes sections for language, description, context,
 * constraints, and other directives specified in the CQL input.
 */
class QueryCompiler final : public QueryVisitor {
public:
    //------------------------------------------------------------------------------
    // Visitor interface implementation
    //------------------------------------------------------------------------------
    
    /**
     * @brief Process a CodeRequestNode
     * @param node The node containing language and description
     */
    void visit(const CodeRequestNode& node) override;
    
    /**
     * @brief Process a ContextNode
     * @param node The node containing context information
     */
    void visit(const ContextNode& node) override;
    
    /**
     * @brief Process a TestNode
     * @param node The node containing test requirements
     */
    void visit(const TestNode& node) override;
    
    /**
     * @brief Process a DependencyNode
     * @param node The node containing dependencies
     */
    void visit(const DependencyNode& node) override;
    
    /**
     * @brief Process a PerformanceNode
     * @param node The node containing performance requirements
     */
    void visit(const PerformanceNode& node) override;
    
    /**
     * @brief Process a CopyrightNode
     * @param node The node containing copyright information
     */
    void visit(const CopyrightNode& node) override;
    
    /**
     * @brief Process an ArchitectureNode
     * @param node The node containing architecture patterns
     */
    void visit(const ArchitectureNode& node) override;
    
    /**
     * @brief Process a ConstraintNode
     * @param node The node containing constraints
     */
    void visit(const ConstraintNode& node) override;
    
    /**
     * @brief Process an ExampleNode
     * @param node The node containing examples
     */
    void visit(const ExampleNode& node) override;
    
    /**
     * @brief Process a SecurityNode
     * @param node The node containing security requirements
     */
    void visit(const SecurityNode& node) override;
    
    /**
     * @brief Process a ComplexityNode
     * @param node The node containing complexity requirements
     */
    void visit(const ComplexityNode& node) override;
    
    /**
     * @brief Process a ModelNode
     * @param node The node containing target model information
     */
    void visit(const ModelNode& node) override;
    
    /**
     * @brief Process a FormatNode
     * @param node The node containing output format information
     */
    void visit(const FormatNode& node) override;
    
    /**
     * @brief Process a VariableNode
     * @param node The node containing variable definition
     */
    void visit(const VariableNode& node) override;

    //------------------------------------------------------------------------------
    // Output methods
    //------------------------------------------------------------------------------
    
    /**
     * @brief Get the final compiled query string
     * 
     * Assembles all sections into a coherent query string formatted according
     * to the specified output format (markdown, json, etc.)
     * 
     * @return The complete formatted query
     */
    [[nodiscard]] std::string get_compiled_query() const;
    
    /**
     * @brief Print the compiled query to an output stream
     * 
     * @param out The output stream (defaults to cout)
     */
    void print_compiled_query(std::ostream& out = std::cout) const;

private:
    // Output content organized by section
    std::map<std::string, std::string> m_result_sections;   ///< Main content sections
    std::vector<std::string> m_test_cases;                  ///< Test requirements
    std::vector<std::pair<std::string, std::string>> m_examples;  ///< Code examples with labels
    
    // Output configuration
    std::string m_target_model = "claude-3-opus";   ///< Target LLM model
    std::string m_output_format = "markdown";       ///< Output format
    
    // Template variables for string interpolation
    std::map<std::string, std::string> m_variables; ///< Variable name to value mapping
    
    /**
     * @brief Process string with variable interpolation
     * 
     * Replaces variable references (${variable_name}) with their values
     * from the m_variables map.
     * 
     * @param input The input string containing variable references
     * @return The processed string with variables replaced
     */
    [[nodiscard]] std::string interpolate_variables(const std::string& input) const;
};

} // namespace cql

#endif // cql_compiler_hpp
