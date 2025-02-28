// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_VISITOR_HPP
#define CQL_VISITOR_HPP

namespace cql {

// Forward declarations for all node types
class CodeRequestNode;
class ContextNode;
class TestNode;
class DependencyNode;
class PerformanceNode;
class CopyrightNode;
class ArchitectureNode;
class ConstraintNode;
class ExampleNode;
class SecurityNode;
class ComplexityNode;
class ModelNode;
class FormatNode;
class VariableNode;

/**
 * @class QueryVisitor
 * @brief Visitor pattern interface for traversing the AST nodes
 *
 * The QueryVisitor class implements the Visitor design pattern, which allows
 * operations to be performed on the AST nodes without modifying the node classes
 * themselves (following the Open/Closed Principle from SOLID).
 *
 * Concrete visitors can implement operations like:
 * - Query compilation (translating the AST to a formatted query)
 * - Validation (checking that the AST satisfies language requirements)
 * - Optimization (rewriting parts of the AST for better results)
 * - Documentation generation (generating documentation from the AST)
 *
 * Each node type has a corresponding visit method that must be implemented
 * by concrete visitor classes.
 */
class QueryVisitor {
public:
    /**
     * @brief Virtual destructor to allow proper cleanup in derived classes
     */
    virtual ~QueryVisitor() = default;
    
    /**
     * @brief Visit a CodeRequestNode
     * @param node The node to visit
     */
    virtual void visit(const CodeRequestNode& node) = 0;
    
    /**
     * @brief Visit a ContextNode
     * @param node The node to visit
     */
    virtual void visit(const ContextNode& node) = 0;
    
    /**
     * @brief Visit a TestNode
     * @param node The node to visit
     */
    virtual void visit(const TestNode& node) = 0;
    
    /**
     * @brief Visit a DependencyNode
     * @param node The node to visit
     */
    virtual void visit(const DependencyNode& node) = 0;
    
    /**
     * @brief Visit a PerformanceNode
     * @param node The node to visit
     */
    virtual void visit(const PerformanceNode& node) = 0;
    
    /**
     * @brief Visit a CopyrightNode
     * @param node The node to visit
     */
    virtual void visit(const CopyrightNode& node) = 0;
    
    //------------------------------------------------------------------------------
    // Phase 2 directive support
    //------------------------------------------------------------------------------
    
    /**
     * @brief Visit an ArchitectureNode
     * @param node The node to visit
     */
    virtual void visit(const ArchitectureNode& node) = 0;
    
    /**
     * @brief Visit a ConstraintNode
     * @param node The node to visit
     */
    virtual void visit(const ConstraintNode& node) = 0;
    
    /**
     * @brief Visit an ExampleNode
     * @param node The node to visit
     */
    virtual void visit(const ExampleNode& node) = 0;
    
    /**
     * @brief Visit a SecurityNode
     * @param node The node to visit
     */
    virtual void visit(const SecurityNode& node) = 0;
    
    /**
     * @brief Visit a ComplexityNode
     * @param node The node to visit
     */
    virtual void visit(const ComplexityNode& node) = 0;
    
    /**
     * @brief Visit a ModelNode
     * @param node The node to visit
     */
    virtual void visit(const ModelNode& node) = 0;
    
    /**
     * @brief Visit a FormatNode
     * @param node The node to visit
     */
    virtual void visit(const FormatNode& node) = 0;
    
    /**
     * @brief Visit a VariableNode
     * @param node The node to visit
     */
    virtual void visit(const VariableNode& node) = 0;
};

} // namespace cql

#endif // cql_visitor_hpp
