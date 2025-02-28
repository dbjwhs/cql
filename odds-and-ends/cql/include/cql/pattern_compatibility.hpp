// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_PATTERN_COMPATIBILITY_HPP
#define CQL_PATTERN_COMPATIBILITY_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <stdexcept>
#include "nodes.hpp"

namespace cql {

/**
 * @class Pattern
 * @brief Represents an architecture design pattern with its properties
 *
 * The Pattern class encapsulates the concept of a software design pattern within the CQL
 * architecture system. Patterns are organized into three layers (Foundation, Component, 
 * and Interaction) and include a name and optional parameters.
 *
 * Patterns can be created either from an ArchitectureNode (parsed from user input)
 * or manually by specifying the layer, name, and parameters.
 *
 * Examples of patterns:
 * - Foundation: MVC, layered, microservices
 * - Component: factory_method, singleton, builder
 * - Interaction: observer, visitor, mediator
 */
class Pattern {
public:
    /**
     * @brief Create a pattern from an architecture node
     * 
     * Extracts the pattern information from an ArchitectureNode, handling both
     * the structured layer:pattern format and the legacy format.
     *
     * @param node The architecture node containing the pattern information
     * @throws std::runtime_error If the node cannot be parsed into a valid pattern
     */
    explicit Pattern(const ArchitectureNode& node);
    
    /**
     * @brief Create a pattern manually with explicit parameters
     * 
     * @param layer The architectural layer (FOUNDATION, COMPONENT, INTERACTION)
     * @param name The pattern name (e.g., "factory_method", "observer")
     * @param parameters Optional configuration parameters for the pattern
     */
    Pattern(PatternLayer layer, const std::string& name, 
            const std::string& parameters = "");
    
    /**
     * @brief Get the pattern's architectural layer
     * 
     * @return The PatternLayer enum value
     */
    [[nodiscard]] PatternLayer get_layer() const { return m_layer; }
    
    /**
     * @brief Get the pattern name
     * 
     * @return The pattern name (e.g., "factory_method", "observer")
     */
    [[nodiscard]] const std::string& get_name() const { return m_name; }
    
    /**
     * @brief Get the pattern parameters
     * 
     * @return The parameter string associated with this pattern
     */
    [[nodiscard]] const std::string& get_parameters() const { return m_parameters; }
    
    /**
     * @brief Get a human-readable string representation of the pattern
     * 
     * @return Formatted string in the form "layer:name (parameters)"
     */
    [[nodiscard]] std::string to_string() const;
    
private:
    PatternLayer m_layer;    ///< The architectural layer this pattern belongs to
    std::string m_name;      ///< Pattern name (e.g., "factory_method", "observer")
    std::string m_parameters; ///< Optional configuration parameters
};

/**
 * @struct CompatibilityIssue
 * @brief Represents an incompatibility between two architectural patterns
 *
 * This structure captures the details of a compatibility issue between patterns,
 * including a descriptive message and string representations of the conflicting patterns.
 */
struct CompatibilityIssue {
    std::string message;   ///< Description of the compatibility issue
    std::string pattern1;  ///< String representation of first pattern
    std::string pattern2;  ///< String representation of second pattern
    
    /**
     * @brief Create a compatibility issue between two patterns
     * 
     * @param msg The error message describing the incompatibility
     * @param p1 The first pattern involved in the conflict
     * @param p2 The second pattern involved in the conflict
     */
    CompatibilityIssue(std::string msg, const Pattern& p1, const Pattern& p2);
    
    /**
     * @brief Get a formatted string representation of the issue
     * 
     * @return A string in the format "message: pattern1 and pattern2"
     */
    [[nodiscard]] std::string to_string() const;
};

/**
 * @class PatternCompatibilityManager
 * @brief Manages compatibility relationships between architectural design patterns
 *
 * This class maintains a comprehensive rule set for determining which design patterns
 * can be used together in the same architecture. The compatibility system enforces:
 *
 * 1. Layer-based rules: Generally, patterns in different layers can coexist
 * 2. Only one Foundation pattern allowed in an architecture
 * 3. Pattern-specific compatibility rules for the 23 standard GoF design patterns
 *    (Creational, Structural, and Behavioral categories)
 *
 * The compatibility rules are organized by pattern category and initialized at construction.
 */
class PatternCompatibilityManager {
public:
    /**
     * @brief Create a new pattern compatibility manager with predefined rules
     *
     * Initializes the compatibility rules for all 23 standard design patterns,
     * organized into creational, structural, and behavioral categories.
     */
    PatternCompatibilityManager();
    
    /**
     * @brief Check if all patterns in a set are compatible with each other
     * 
     * Performs a comprehensive analysis of pattern compatibility, checking:
     * 1. No multiple Foundation patterns
     * 2. No incompatible patterns within the same layer
     * 
     * @param patterns The set of patterns to check for compatibility
     * @return List of compatibility issues (empty if all patterns are compatible)
     */
    [[nodiscard]] std::vector<CompatibilityIssue> check_compatibility(
        const std::vector<Pattern>& patterns) const;
    
    /**
     * @brief Check if all architecture nodes represent compatible patterns
     * 
     * Convenience method that extracts patterns from architecture nodes and
     * performs compatibility checking.
     * 
     * @param nodes Vector of architecture nodes to check
     * @return List of compatibility issues (empty if all patterns are compatible)
     */
    [[nodiscard]] std::vector<CompatibilityIssue> check_compatibility(
        const std::vector<const ArchitectureNode*>& nodes) const;
    
    /**
     * @brief Check if two specific patterns are compatible
     * 
     * Determines if two patterns can be used together based on:
     * 1. Layer compatibility (different layers are always compatible)
     * 2. Foundation layer exclusivity (only one foundation pattern allowed)
     * 3. Pattern-specific compatibility rules
     * 
     * @param p1 First pattern to check
     * @param p2 Second pattern to check
     * @return true if patterns are compatible, false otherwise
     */
    [[nodiscard]] bool are_patterns_compatible(
        const Pattern& p1, const Pattern& p2) const;
    
private:
    /**
     * @struct CompatibilityRule
     * @brief Defines compatibility relationships for a specific pattern
     */
    struct CompatibilityRule {
        std::string pattern_name;                  ///< The pattern this rule applies to
        std::set<std::string> compatible_patterns; ///< Patterns explicitly marked as compatible
        std::set<std::string> incompatible_patterns; ///< Patterns explicitly marked as incompatible
    };
    
    /// Maps pattern names to their compatibility rules
    std::map<std::string, CompatibilityRule> m_compatibility_rules;
    
    /**
     * @brief Initialize compatibility rules for all creational patterns
     * 
     * Sets up rules for: factory_method, abstract_factory, builder, singleton, prototype
     */
    void initialize_creational_patterns();
    
    /**
     * @brief Initialize compatibility rules for all structural patterns
     * 
     * Sets up rules for: adapter, bridge, composite, decorator, facade, flyweight, proxy
     */
    void initialize_structural_patterns();
    
    /**
     * @brief Initialize compatibility rules for all behavioral patterns
     * 
     * Sets up rules for: chain, command, interpreter, iterator, mediator, memento,
     * observer, state, strategy, template_method, visitor
     */
    void initialize_behavioral_patterns();
};

} // namespace cql

#endif // CQL_PATTERN_COMPATIBILITY_HPP