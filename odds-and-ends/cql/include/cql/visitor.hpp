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
};

} // namespace cql

#endif // CQL_VISITOR_HPP