// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/nodes.hpp"
#include "../../include/cql/visitor.hpp"

namespace cql {

// CodeRequestNode implementation
CodeRequestNode::CodeRequestNode(std::string language, std::string description)
    : m_language(std::move(language)), m_description(std::move(description)) {}

void CodeRequestNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& CodeRequestNode::language() const { 
    return m_language; 
}

const std::string& CodeRequestNode::description() const { 
    return m_description; 
}

// ContextNode implementation
ContextNode::ContextNode(std::string context)
    : m_context(std::move(context)) {}

void ContextNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& ContextNode::context() const { 
    return m_context; 
}

// TestNode implementation
TestNode::TestNode(std::vector<std::string> test_cases)
    : m_test_cases(std::move(test_cases)) {}

void TestNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::vector<std::string>& TestNode::test_cases() const { 
    return m_test_cases; 
}

// DependencyNode implementation
DependencyNode::DependencyNode(std::vector<std::string> dependencies)
    : m_dependencies(std::move(dependencies)) {}

void DependencyNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::vector<std::string>& DependencyNode::dependencies() const { 
    return m_dependencies; 
}

// PerformanceNode implementation
PerformanceNode::PerformanceNode(std::string requirement)
    : m_requirement(std::move(requirement)) {}

void PerformanceNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& PerformanceNode::requirement() const { 
    return m_requirement; 
}

// CopyrightNode implementation
CopyrightNode::CopyrightNode(std::string license, std::string owner)
    : m_license(std::move(license)), m_owner(std::move(owner)) {}

void CopyrightNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& CopyrightNode::license() const { 
    return m_license; 
}

const std::string& CopyrightNode::owner() const { 
    return m_owner; 
}

} // namespace cql