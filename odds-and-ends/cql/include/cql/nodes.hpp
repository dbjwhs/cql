// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_NODES_HPP
#define CQL_NODES_HPP

#include <string>
#include <vector>
#include <memory>

namespace cql {

// forward declarations
class QueryVisitor;

/**
 * base class for all query nodes in the abstract syntax tree (ast)
 */
class QueryNode {
public:
    virtual ~QueryNode() = default;
    virtual void accept(QueryVisitor& visitor) const = 0;
};

/**
 * node for code generation requests (@language and @description directives)
 */
class CodeRequestNode final : public QueryNode {
public:
    explicit CodeRequestNode(std::string language, std::string description);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& language() const;
    [[nodiscard]] const std::string& description() const;

private:
    std::string m_language;
    std::string m_description;
};

/**
 * node for providing context about the code (@context directive)
 */
class ContextNode final : public QueryNode {
public:
    explicit ContextNode(std::string context);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& context() const;

private:
    std::string m_context;
};

/**
 * node for specifying test requirements (@test directive)
 */
class TestNode final : public QueryNode {
public:
    explicit TestNode(std::vector<std::string> test_cases);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::vector<std::string>& test_cases() const;

private:
    std::vector<std::string> m_test_cases;
};

/**
 * node for specifying dependencies (@dependency directive)
 */
class DependencyNode final : public QueryNode {
public:
    explicit DependencyNode(std::vector<std::string> dependencies);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::vector<std::string>& dependencies() const;

private:
    std::vector<std::string> m_dependencies;
};

/**
 * node for specifying performance requirements (@performance directive)
 */
class PerformanceNode final : public QueryNode {
public:
    explicit PerformanceNode(std::string requirement);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& requirement() const;

private:
    std::string m_requirement;
};

/**
 * node for specifying copyright and license (@copyright directive)
 */
class CopyrightNode final : public QueryNode {
public:
    explicit CopyrightNode(std::string license, std::string owner);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& license() const;
    [[nodiscard]] const std::string& owner() const;

private:
    std::string m_license;
    std::string m_owner;
};

/**
 * node for specifying system architecture (@architecture directive)
 */
class ArchitectureNode final : public QueryNode {
public:
    explicit ArchitectureNode(std::string architecture);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& architecture() const;

private:
    std::string m_architecture;
};

/**
 * node for specifying constraints (@constraint directive)
 */
class ConstraintNode final : public QueryNode {
public:
    explicit ConstraintNode(std::string constraint);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& constraint() const;

private:
    std::string m_constraint;
};

/**
 * node for providing code examples (@example directive)
 */
class ExampleNode final : public QueryNode {
public:
    explicit ExampleNode(std::string label, std::string code);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& label() const;
    [[nodiscard]] const std::string& code() const;

private:
    std::string m_label;
    std::string m_code;
};

/**
 * node for specifying security requirements (@security directive)
 */
class SecurityNode final : public QueryNode {
public:
    explicit SecurityNode(std::string requirement);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& requirement() const;

private:
    std::string m_requirement;
};

/**
 * node for specifying algorithm complexity requirements (@complexity directive)
 */
class ComplexityNode final : public QueryNode {
public:
    explicit ComplexityNode(std::string complexity);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& complexity() const;

private:
    std::string m_complexity;
};

/**
 * node for specifying target llm model (@model directive)
 */
class ModelNode final : public QueryNode {
public:
    explicit ModelNode(std::string model_name);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& model_name() const;

private:
    std::string m_model_name;
};

/**
 * node for specifying output format (@format directive)
 */
class FormatNode final : public QueryNode {
public:
    explicit FormatNode(std::string format_type);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& format_type() const;

private:
    std::string m_format_type;
};

/**
 * node for declaring template variables (@variable directive)
 */
class VariableNode final : public QueryNode {
public:
    explicit VariableNode(std::string name, std::string value);
    void accept(QueryVisitor& visitor) const override;

    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] const std::string& value() const;

private:
    std::string m_name;
    std::string m_value;
};

} // namespace cql

#endif // cql_nodes_hpp
