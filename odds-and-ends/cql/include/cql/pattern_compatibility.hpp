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
 * Represents an architecture design pattern with its properties
 */
class Pattern {
public:
    /**
     * Create a pattern from a node
     * @param node The architecture node containing the pattern
     */
    explicit Pattern(const ArchitectureNode& node);
    
    /**
     * Create a pattern manually
     * @param layer The pattern layer
     * @param name The pattern name
     * @param parameters Optional pattern parameters
     */
    Pattern(PatternLayer layer, const std::string& name, 
            const std::string& parameters = "");
    
    /**
     * Get the pattern layer
     * @return The pattern layer
     */
    [[nodiscard]] PatternLayer get_layer() const { return m_layer; }
    
    /**
     * Get the pattern name
     * @return The pattern name
     */
    [[nodiscard]] const std::string& get_name() const { return m_name; }
    
    /**
     * Get the pattern parameters
     * @return The pattern parameters
     */
    [[nodiscard]] const std::string& get_parameters() const { return m_parameters; }
    
    /**
     * Get a human-readable string representation of the pattern
     * @return String representation
     */
    [[nodiscard]] std::string to_string() const;
    
private:
    PatternLayer m_layer;
    std::string m_name;
    std::string m_parameters;
};

/**
 * Represents an issue with pattern compatibility
 */
struct CompatibilityIssue {
    std::string message;
    std::string pattern1;
    std::string pattern2;
    
    CompatibilityIssue(std::string msg, const Pattern& p1, const Pattern& p2);
    
    [[nodiscard]] std::string to_string() const;
};

/**
 * Manages pattern compatibility checking
 */
class PatternCompatibilityManager {
public:
    /**
     * Create a new pattern compatibility manager
     */
    PatternCompatibilityManager();
    
    /**
     * Check if all patterns in a set are compatible with each other
     * @param patterns The set of patterns to check
     * @return List of compatibility issues, empty if all patterns are compatible
     */
    [[nodiscard]] std::vector<CompatibilityIssue> check_compatibility(
        const std::vector<Pattern>& patterns) const;
    
    /**
     * Check if all patterns in a set of architecture nodes are compatible
     * @param nodes The architecture nodes to check
     * @return List of compatibility issues, empty if all patterns are compatible
     */
    [[nodiscard]] std::vector<CompatibilityIssue> check_compatibility(
        const std::vector<const ArchitectureNode*>& nodes) const;
    
    /**
     * Check if two patterns are compatible
     * @param p1 First pattern
     * @param p2 Second pattern
     * @return True if patterns are compatible, false otherwise
     */
    [[nodiscard]] bool are_patterns_compatible(
        const Pattern& p1, const Pattern& p2) const;
    
private:
    // Pattern compatibility rules
    struct CompatibilityRule {
        std::string pattern_name;
        std::set<std::string> compatible_patterns;
        std::set<std::string> incompatible_patterns;
    };
    
    // Maps pattern names to their compatibility rules
    std::map<std::string, CompatibilityRule> m_compatibility_rules;
    
    // Initialize the compatibility rules for creational patterns (Phase 1)
    void initialize_creational_patterns();
};

} // namespace cql

#endif // CQL_PATTERN_COMPATIBILITY_HPP