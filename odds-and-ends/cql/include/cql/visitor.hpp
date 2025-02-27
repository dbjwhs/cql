// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_VISITOR_HPP
#define CQL_VISITOR_HPP

namespace cql {

// Forward declarations
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
 * Visitor pattern interface for traversing the AST nodes
 * Used to implement different operations on the query tree
 * without modifying the node classes (Open/Closed Principle)
 */
class QueryVisitor {
public:
    virtual ~QueryVisitor() = default;
    virtual void visit(const CodeRequestNode& node) = 0;
    virtual void visit(const ContextNode& node) = 0;
    virtual void visit(const TestNode& node) = 0;
    virtual void visit(const DependencyNode& node) = 0;
    virtual void visit(const PerformanceNode& node) = 0;
    virtual void visit(const CopyrightNode& node) = 0;
    
    // Phase 2 directive support
    virtual void visit(const ArchitectureNode& node) = 0;
    virtual void visit(const ConstraintNode& node) = 0;
    virtual void visit(const ExampleNode& node) = 0;
    virtual void visit(const SecurityNode& node) = 0;
    virtual void visit(const ComplexityNode& node) = 0;
    virtual void visit(const ModelNode& node) = 0;
    virtual void visit(const FormatNode& node) = 0;
    virtual void visit(const VariableNode& node) = 0;
};

} // namespace cql

#endif // CQL_VISITOR_HPP