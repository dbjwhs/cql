// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_NODES_HPP
#define CQL_NODES_HPP

#include <string>
#include <vector>
#include <memory>

namespace cql {

// Forward declarations
class QueryVisitor;

/**
 * @class QueryNode
 * @brief Base abstract class for all query nodes in the abstract syntax tree (AST)
 *
 * The QueryNode class serves as the foundation for the visitor pattern implementation
 * in the CQL compiler. All concrete node types derive from this base class and must
 * implement the accept() method to enable traversal by visitor objects.
 *
 * Each concrete node type represents a specific directive in the CQL language,
 * such as @language, @context, @test, etc.
 */
class QueryNode {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup in derived classes
     */
    virtual ~QueryNode() = default;
    
    /**
     * @brief Accept a visitor to process this node
     * 
     * This method implements the Visitor pattern, allowing operations to be 
     * performed on nodes without modifying their classes.
     * 
     * @param visitor The visitor object that will process this node
     */
    virtual void accept(QueryVisitor& visitor) const = 0;
};

/**
 * @class CodeRequestNode
 * @brief Node for code generation requests (@language and @description directives)
 *
 * Represents the core code generation request, which includes:
 * - The target programming language (@language directive)
 * - A description of the code to be generated (@description directive)
 *
 * This node is mandatory for all CQL queries and forms the primary instruction
 * to the LLM for code generation.
 *
 * Example:
 * @language cpp
 * @description Implement a binary search algorithm
 */
class CodeRequestNode final : public QueryNode {
public:
    /**
     * @brief Construct a code request node
     * 
     * @param language Target programming language (e.g., "cpp", "python", "javascript")
     * @param description Description of the code to be generated
     */
    explicit CodeRequestNode(std::string language, std::string description);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the target programming language
     * @return The language string (e.g., "cpp", "python", "javascript")
     */
    [[nodiscard]] const std::string& language() const;
    
    /**
     * @brief Get the code description
     * @return The description of the code to be generated
     */
    [[nodiscard]] const std::string& description() const;

private:
    std::string m_language;    ///< Target programming language
    std::string m_description; ///< Description of the code to be generated
};

/**
 * @class ContextNode
 * @brief Node for providing context about the code (@context directive)
 *
 * Provides additional context or background information to help the LLM
 * understand the requirements better.
 *
 * Example:
 * @context This will be used in a real-time system with strict performance requirements.
 */
class ContextNode final : public QueryNode {
public:
    /**
     * @brief Construct a context node
     * 
     * @param context Additional context information for the code generation
     */
    explicit ContextNode(std::string context);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the context information
     * @return The context string
     */
    [[nodiscard]] const std::string& context() const;

private:
    std::string m_context; ///< The context information
};

/**
 * @class TestNode
 * @brief Node for specifying test requirements (@test directive)
 *
 * Provides test cases or test requirements for the generated code.
 * Multiple test cases can be specified.
 *
 * Example:
 * @test The function should handle empty arrays correctly
 * @test Performance should be O(log n)
 */
class TestNode final : public QueryNode {
public:
    /**
     * @brief Construct a test node with multiple test cases
     * 
     * @param test_cases Vector of test cases or test requirements
     */
    explicit TestNode(std::vector<std::string> test_cases);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the test cases
     * @return Vector of test case strings
     */
    [[nodiscard]] const std::vector<std::string>& test_cases() const;

private:
    std::vector<std::string> m_test_cases; ///< List of test cases
};

/**
 * @class DependencyNode
 * @brief Node for specifying dependencies (@dependency directive)
 *
 * Lists external libraries, frameworks, or packages that the generated
 * code should use or be compatible with.
 *
 * Example:
 * @dependency boost
 * @dependency eigen
 */
class DependencyNode final : public QueryNode {
public:
    /**
     * @brief Construct a dependency node with multiple dependencies
     * 
     * @param dependencies Vector of dependency names
     */
    explicit DependencyNode(std::vector<std::string> dependencies);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the dependencies
     * @return Vector of dependency strings
     */
    [[nodiscard]] const std::vector<std::string>& dependencies() const;

private:
    std::vector<std::string> m_dependencies; ///< List of dependencies
};

/**
 * @class PerformanceNode
 * @brief Node for specifying performance requirements (@performance directive)
 *
 * Defines performance requirements for the generated code,
 * such as execution time, memory usage, or scaling characteristics.
 *
 * Example:
 * @performance The function should complete in under 10ms for inputs up to 1000 elements
 */
class PerformanceNode final : public QueryNode {
public:
    /**
     * @brief Construct a performance node
     * 
     * @param requirement The performance requirement description
     */
    explicit PerformanceNode(std::string requirement);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the performance requirement
     * @return The performance requirement string
     */
    [[nodiscard]] const std::string& requirement() const;

private:
    std::string m_requirement; ///< The performance requirement
};

/**
 * @class CopyrightNode
 * @brief Node for specifying copyright and license (@copyright directive)
 *
 * Defines the copyright holder and license to be applied to the generated code.
 *
 * Example:
 * @copyright MIT Jane Doe
 */
class CopyrightNode final : public QueryNode {
public:
    /**
     * @brief Construct a copyright node
     * 
     * @param license License name (e.g., "MIT", "GPL", "Apache-2.0")
     * @param owner Copyright owner's name
     */
    explicit CopyrightNode(std::string license, std::string owner);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the license type
     * @return The license string (e.g., "MIT", "GPL")
     */
    [[nodiscard]] const std::string& license() const;
    
    /**
     * @brief Get the copyright owner
     * @return The copyright owner's name
     */
    [[nodiscard]] const std::string& owner() const;

private:
    std::string m_license; ///< The license type
    std::string m_owner;   ///< The copyright owner
};

/**
 * @enum PatternLayer
 * @brief Represents the architectural layer of a design pattern
 *
 * Design patterns are organized into three layers:
 * - FOUNDATION: Core architectural patterns that define the overall system structure
 * - COMPONENT: Patterns that define how individual components are structured
 * - INTERACTION: Patterns that govern how components interact with each other
 */
enum class PatternLayer {
    FOUNDATION,   ///< Core architectural patterns (e.g., MVC, layered, microservices)
    COMPONENT,    ///< Component-level patterns (e.g., Factory, Singleton)
    INTERACTION   ///< Patterns governing component interactions (e.g., Observer, Visitor)
};

/**
 * @brief Converts a PatternLayer enum to string representation
 * 
 * @param layer The PatternLayer to convert
 * @return String representation ("foundation", "component", or "interaction")
 */
std::string pattern_layer_to_string(PatternLayer layer);

/**
 * @brief Converts a string to PatternLayer enum
 * 
 * @param layer_str The string to convert ("foundation", "component", or "interaction")
 * @return Corresponding PatternLayer enum value
 * @throws std::invalid_argument If the string doesn't match a valid layer
 */
PatternLayer string_to_pattern_layer(const std::string& layer_str);

/**
 * @class ArchitectureNode
 * @brief Node for specifying system architecture (@architecture directive)
 *
 * Defines the architectural design patterns to be used in the generated code.
 * Patterns can be specified in either:
 * - Legacy format: @architecture singleton
 * - Layered format: @architecture component:factory_method parameterized=true
 *
 * Examples:
 * @architecture component:factory_method
 * @architecture interaction:observer
 * @architecture foundation:mvc
 */
class ArchitectureNode final : public QueryNode {
public:
    /**
     * @brief Construct an architecture node with legacy format
     * 
     * @param architecture The architecture description in legacy format
     */
    explicit ArchitectureNode(std::string architecture);
    
    /**
     * @brief Construct an architecture node with layered format
     * 
     * @param layer The pattern layer (FOUNDATION, COMPONENT, INTERACTION)
     * @param pattern_name The pattern name (e.g., "factory_method", "observer")
     * @param parameters Optional configuration parameters
     */
    explicit ArchitectureNode(PatternLayer layer, std::string pattern_name, std::string parameters);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the raw architecture string (for legacy format)
     * @return The architecture string
     */
    [[nodiscard]] const std::string& architecture() const;
    
    /**
     * @brief Get the pattern layer
     * @return The PatternLayer enum value
     */
    [[nodiscard]] PatternLayer get_layer() const;
    
    /**
     * @brief Get the pattern name
     * @return The pattern name string
     */
    [[nodiscard]] const std::string& get_pattern_name() const;
    
    /**
     * @brief Get the pattern parameters
     * @return The parameters string
     */
    [[nodiscard]] const std::string& get_parameters() const;
    
    /**
     * @brief Check if the node uses the layered format
     * @return true if using layer:pattern format, false if using legacy format
     */
    [[nodiscard]] bool is_layered_format() const;

private:
    std::string m_architecture;  ///< Raw architecture string (for legacy format)
    PatternLayer m_layer{PatternLayer::COMPONENT}; ///< Pattern layer (default: COMPONENT)
    std::string m_pattern_name;  ///< Pattern name (for layered format)
    std::string m_parameters;    ///< Pattern parameters (for layered format)
    bool m_is_layered_format{false}; ///< Flag indicating format type
};

/**
 * @class ConstraintNode
 * @brief Node for specifying constraints (@constraint directive)
 *
 * Defines constraints that the generated code must satisfy.
 *
 * Example:
 * @constraint The implementation must not use recursion
 */
class ConstraintNode final : public QueryNode {
public:
    /**
     * @brief Construct a constraint node
     * 
     * @param constraint The constraint description
     */
    explicit ConstraintNode(std::string constraint);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the constraint
     * @return The constraint string
     */
    [[nodiscard]] const std::string& constraint() const;

private:
    std::string m_constraint; ///< The constraint description
};

/**
 * @class ExampleNode
 * @brief Node for providing code examples (@example directive)
 *
 * Provides example code to guide the LLM's generation.
 * Examples can include a label for clarity.
 *
 * Example:
 * @example function_usage
 * auto result = binary_search(vec, target);
 * if (result.has_value()) {
 *     std::cout << "Found at index: " << *result << std::endl;
 * }
 */
class ExampleNode final : public QueryNode {
public:
    /**
     * @brief Construct an example node
     * 
     * @param label Optional label for the example
     * @param code The example code
     */
    explicit ExampleNode(std::string label, std::string code);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the example label
     * @return The label string
     */
    [[nodiscard]] const std::string& label() const;
    
    /**
     * @brief Get the example code
     * @return The code string
     */
    [[nodiscard]] const std::string& code() const;

private:
    std::string m_label; ///< The example label
    std::string m_code;  ///< The example code
};

/**
 * @class SecurityNode
 * @brief Node for specifying security requirements (@security directive)
 *
 * Defines security requirements or concerns for the generated code.
 *
 * Example:
 * @security The function must validate all inputs to prevent buffer overflows
 */
class SecurityNode final : public QueryNode {
public:
    /**
     * @brief Construct a security node
     * 
     * @param requirement The security requirement description
     */
    explicit SecurityNode(std::string requirement);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the security requirement
     * @return The security requirement string
     */
    [[nodiscard]] const std::string& requirement() const;

private:
    std::string m_requirement; ///< The security requirement
};

/**
 * @class ComplexityNode
 * @brief Node for specifying algorithm complexity requirements (@complexity directive)
 *
 * Defines the required time or space complexity for the algorithm.
 *
 * Example:
 * @complexity Time: O(n log n), Space: O(1)
 */
class ComplexityNode final : public QueryNode {
public:
    /**
     * @brief Construct a complexity node
     * 
     * @param complexity The complexity requirement description
     */
    explicit ComplexityNode(std::string complexity);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the complexity requirement
     * @return The complexity requirement string
     */
    [[nodiscard]] const std::string& complexity() const;

private:
    std::string m_complexity; ///< The complexity requirement
};

/**
 * @class ModelNode
 * @brief Node for specifying target LLM model (@model directive)
 *
 * Specifies which language model variant to target.
 *
 * Example:
 * @model claude-3-opus-20240229
 */
class ModelNode final : public QueryNode {
public:
    /**
     * @brief Construct a model node
     * 
     * @param model_name The target LLM model name
     */
    explicit ModelNode(std::string model_name);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the model name
     * @return The model name string
     */
    [[nodiscard]] const std::string& model_name() const;

private:
    std::string m_model_name; ///< The target LLM model name
};

/**
 * @class FormatNode
 * @brief Node for specifying output format (@format directive)
 *
 * Defines the desired format for the generated code.
 *
 * Example:
 * @format json
 * @format class_implementation
 */
class FormatNode final : public QueryNode {
public:
    /**
     * @brief Construct a format node
     * 
     * @param format_type The desired output format
     */
    explicit FormatNode(std::string format_type);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the format type
     * @return The format type string
     */
    [[nodiscard]] const std::string& format_type() const;

private:
    std::string m_format_type; ///< The output format type
};

/**
 * @class VariableNode
 * @brief Node for declaring template variables (@variable directive)
 *
 * Defines variables that can be used in templates for substitution.
 *
 * Example:
 * @variable class_name BinarySearchTree
 * @variable value_type int
 */
class VariableNode final : public QueryNode {
public:
    /**
     * @brief Construct a variable node
     * 
     * @param name The variable name
     * @param value The variable value
     */
    explicit VariableNode(std::string name, std::string value);
    
    /**
     * @brief Accept a visitor to process this node
     * @param visitor The visitor object
     */
    void accept(QueryVisitor& visitor) const override;

    /**
     * @brief Get the variable name
     * @return The variable name string
     */
    [[nodiscard]] const std::string& name() const;
    
    /**
     * @brief Get the variable value
     * @return The variable value string
     */
    [[nodiscard]] const std::string& value() const;

private:
    std::string m_name;  ///< The variable name
    std::string m_value; ///< The variable value
};

} // namespace cql

#endif // cql_nodes_hpp
