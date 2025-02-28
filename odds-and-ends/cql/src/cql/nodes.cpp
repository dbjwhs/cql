// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/nodes.hpp"
#include "../../include/cql/visitor.hpp"

namespace cql {

// coderequestnode implementation
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

// contextnode implementation
ContextNode::ContextNode(std::string context)
    : m_context(std::move(context)) {}

void ContextNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& ContextNode::context() const { 
    return m_context; 
}

// testnode implementation
TestNode::TestNode(std::vector<std::string> test_cases)
    : m_test_cases(std::move(test_cases)) {}

void TestNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::vector<std::string>& TestNode::test_cases() const { 
    return m_test_cases; 
}

// dependencynode implementation
DependencyNode::DependencyNode(std::vector<std::string> dependencies)
    : m_dependencies(std::move(dependencies)) {}

void DependencyNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::vector<std::string>& DependencyNode::dependencies() const { 
    return m_dependencies; 
}

// performancenode implementation
PerformanceNode::PerformanceNode(std::string requirement)
    : m_requirement(std::move(requirement)) {}

void PerformanceNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& PerformanceNode::requirement() const { 
    return m_requirement; 
}

// copyrightnode implementation
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

// architecturenode implementation
ArchitectureNode::ArchitectureNode(std::string architecture)
    : m_architecture(std::move(architecture)) {}

void ArchitectureNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& ArchitectureNode::architecture() const { 
    return m_architecture; 
}

// constraintnode implementation
ConstraintNode::ConstraintNode(std::string constraint)
    : m_constraint(std::move(constraint)) {}

void ConstraintNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& ConstraintNode::constraint() const { 
    return m_constraint; 
}

// examplenode implementation
ExampleNode::ExampleNode(std::string label, std::string code)
    : m_label(std::move(label)), m_code(std::move(code)) {}

void ExampleNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& ExampleNode::label() const { 
    return m_label; 
}

const std::string& ExampleNode::code() const { 
    return m_code; 
}

// securitynode implementation
SecurityNode::SecurityNode(std::string requirement)
    : m_requirement(std::move(requirement)) {}

void SecurityNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& SecurityNode::requirement() const { 
    return m_requirement; 
}

// complexitynode implementation
ComplexityNode::ComplexityNode(std::string complexity)
    : m_complexity(std::move(complexity)) {}

void ComplexityNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& ComplexityNode::complexity() const { 
    return m_complexity; 
}

// modelnode implementation
ModelNode::ModelNode(std::string model_name)
    : m_model_name(std::move(model_name)) {}

void ModelNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& ModelNode::model_name() const { 
    return m_model_name; 
}

// formatnode implementation
FormatNode::FormatNode(std::string format_type)
    : m_format_type(std::move(format_type)) {}

void FormatNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& FormatNode::format_type() const { 
    return m_format_type; 
}

// variablenode implementation
VariableNode::VariableNode(std::string name, std::string value)
    : m_name(std::move(name)), m_value(std::move(value)) {}

void VariableNode::accept(QueryVisitor& visitor) const {
    visitor.visit(*this);
}

const std::string& VariableNode::name() const { 
    return m_name; 
}

const std::string& VariableNode::value() const { 
    return m_value; 
}

} // namespace cql
