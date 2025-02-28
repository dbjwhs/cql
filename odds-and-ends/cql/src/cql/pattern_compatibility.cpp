// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/pattern_compatibility.hpp"
#include <algorithm>
#include <sstream>

namespace cql {

// Pattern implementation
Pattern::Pattern(const ArchitectureNode& node) {
    if (node.is_layered_format()) {
        m_layer = node.get_layer();
        m_name = node.get_pattern_name();
        m_parameters = node.get_parameters();
    } else {
        // Handle legacy format - try to extract pattern name
        std::string arch = node.architecture();
        
        // Default to component layer for legacy format
        m_layer = PatternLayer::COMPONENT;
        
        // Check if it's one of our known patterns
        static const std::vector<std::string> known_patterns = {
            "factory_method", "abstract_factory", "builder", "singleton", "prototype",
            "adapter", "bridge", "composite", "decorator", "facade", "flyweight", "proxy",
            "chain", "command", "interpreter", "iterator", "mediator", "memento",
            "observer", "state", "strategy", "template_method", "visitor"
        };
        
        for (const auto& pattern : known_patterns) {
            if (arch.find(pattern) != std::string::npos) {
                m_name = pattern;
                m_parameters = ""; // No parameters for legacy format
                return;
            }
        }
        
        // Default to the full string as name if no known pattern is found
        m_name = arch;
        m_parameters = "";
    }
}

Pattern::Pattern(PatternLayer layer, const std::string& name, const std::string& parameters)
    : m_layer(layer), m_name(name), m_parameters(parameters) {
}

std::string Pattern::to_string() const {
    std::ostringstream oss;
    oss << pattern_layer_to_string(m_layer) << ":" << m_name;
    if (!m_parameters.empty()) {
        oss << " (" << m_parameters << ")";
    }
    return oss.str();
}

// CompatibilityIssue implementation
CompatibilityIssue::CompatibilityIssue(std::string msg, const Pattern& p1, const Pattern& p2)
    : message(std::move(msg)), pattern1(p1.to_string()), pattern2(p2.to_string()) {
}

std::string CompatibilityIssue::to_string() const {
    return message + ": " + pattern1 + " and " + pattern2;
}

// PatternCompatibilityManager implementation
PatternCompatibilityManager::PatternCompatibilityManager() {
    // Initialize the compatibility rules
    initialize_creational_patterns();
}

void PatternCompatibilityManager::initialize_creational_patterns() {
    // Factory Method pattern
    CompatibilityRule factory_method;
    factory_method.pattern_name = "factory_method";
    factory_method.compatible_patterns = {
        "abstract_factory", "singleton", "builder", "prototype", 
        "observer", "decorator", "strategy"
    };
    factory_method.incompatible_patterns = {
        // Generally, you shouldn't have multiple factory methods for the same product types
        // But this depends on the specific implementation and context
    };
    m_compatibility_rules["factory_method"] = factory_method;
    
    // Abstract Factory pattern
    CompatibilityRule abstract_factory;
    abstract_factory.pattern_name = "abstract_factory";
    abstract_factory.compatible_patterns = {
        "factory_method", "singleton", "builder", "prototype",
        "observer", "decorator", "strategy"
    };
    abstract_factory.incompatible_patterns = {
        // Similar to factory method, context-dependent
    };
    m_compatibility_rules["abstract_factory"] = abstract_factory;
    
    // Builder pattern
    CompatibilityRule builder;
    builder.pattern_name = "builder";
    builder.compatible_patterns = {
        "factory_method", "abstract_factory", "singleton", 
        "observer", "decorator", "strategy"
    };
    builder.incompatible_patterns = {
        // Builder and prototype can conflict since they both create objects, but in different ways
        // This is context-dependent though
    };
    m_compatibility_rules["builder"] = builder;
    
    // Singleton pattern
    CompatibilityRule singleton;
    singleton.pattern_name = "singleton";
    singleton.compatible_patterns = {
        "factory_method", "abstract_factory", "builder",
        "facade", "proxy", "observer", "strategy"
    };
    singleton.incompatible_patterns = {
        "prototype" // Prototype cloning conflicts with singleton's single instance guarantee
    };
    m_compatibility_rules["singleton"] = singleton;
    
    // Prototype pattern
    CompatibilityRule prototype;
    prototype.pattern_name = "prototype";
    prototype.compatible_patterns = {
        "factory_method", "abstract_factory", 
        "observer", "decorator", "strategy"
    };
    prototype.incompatible_patterns = {
        "singleton" // Singleton conflicts with prototype's cloning behavior
    };
    m_compatibility_rules["prototype"] = prototype;
    
    // Structural patterns - Phase 2
    
    // Adapter pattern
    CompatibilityRule adapter;
    adapter.pattern_name = "adapter";
    adapter.compatible_patterns = {
        "factory_method", "abstract_factory", "builder", "singleton",
        "facade", "bridge", "decorator", "proxy",
        "observer", "strategy", "visitor"
    };
    adapter.incompatible_patterns = {
        // Generally compatible with most patterns
    };
    m_compatibility_rules["adapter"] = adapter;
    
    // Bridge pattern
    CompatibilityRule bridge;
    bridge.pattern_name = "bridge";
    bridge.compatible_patterns = {
        "factory_method", "abstract_factory", "singleton",
        "adapter", "decorator", "proxy",
        "observer", "strategy"
    };
    bridge.incompatible_patterns = {
        // Bridge and composite can conflict in some implementations
        "composite"
    };
    m_compatibility_rules["bridge"] = bridge;
    
    // Composite pattern
    CompatibilityRule composite;
    composite.pattern_name = "composite";
    composite.compatible_patterns = {
        "factory_method", "abstract_factory", "builder", 
        "decorator", "flyweight", "iterator", "visitor"
    };
    composite.incompatible_patterns = {
        "bridge" // Can conflict with bridge in some implementations
    };
    m_compatibility_rules["composite"] = composite;
    
    // Decorator pattern
    CompatibilityRule decorator;
    decorator.pattern_name = "decorator";
    decorator.compatible_patterns = {
        "factory_method", "abstract_factory", "builder", "prototype",
        "adapter", "bridge", "composite", "proxy",
        "observer", "strategy", "template_method"
    };
    decorator.incompatible_patterns = {
        // Generally compatible with most patterns
    };
    m_compatibility_rules["decorator"] = decorator;
    
    // Facade pattern
    CompatibilityRule facade;
    facade.pattern_name = "facade";
    facade.compatible_patterns = {
        "factory_method", "abstract_factory", "singleton",
        "adapter", "proxy", "mediator"
    };
    facade.incompatible_patterns = {
        // Facade can conflict with decorator if overused
        // But this is context-dependent
    };
    m_compatibility_rules["facade"] = facade;
    
    // Flyweight pattern
    CompatibilityRule flyweight;
    flyweight.pattern_name = "flyweight";
    flyweight.compatible_patterns = {
        "factory_method", "singleton",
        "composite", "proxy",
        "observer", "state"
    };
    flyweight.incompatible_patterns = {
        "prototype" // Flyweight shares instances, prototype copies them
    };
    m_compatibility_rules["flyweight"] = flyweight;
    
    // Proxy pattern
    CompatibilityRule proxy;
    proxy.pattern_name = "proxy";
    proxy.compatible_patterns = {
        "factory_method", "abstract_factory", "singleton",
        "adapter", "bridge", "decorator", "facade", "flyweight",
        "observer", "strategy", "chain"
    };
    proxy.incompatible_patterns = {
        // Generally compatible with most patterns
    };
    m_compatibility_rules["proxy"] = proxy;
}

std::vector<CompatibilityIssue> PatternCompatibilityManager::check_compatibility(
    const std::vector<Pattern>& patterns) const {
    
    std::vector<CompatibilityIssue> issues;
    
    // Check for multiple foundation patterns
    std::vector<Pattern> foundation_patterns;
    for (const auto& pattern : patterns) {
        if (pattern.get_layer() == PatternLayer::FOUNDATION) {
            foundation_patterns.push_back(pattern);
        }
    }
    
    if (foundation_patterns.size() > 1) {
        for (size_t i = 0; i < foundation_patterns.size() - 1; ++i) {
            for (size_t j = i + 1; j < foundation_patterns.size(); ++j) {
                issues.emplace_back(
                    "Multiple foundation patterns are not allowed",
                    foundation_patterns[i],
                    foundation_patterns[j]
                );
            }
        }
    }
    
    // Check for pattern compatibility
    for (size_t i = 0; i < patterns.size(); ++i) {
        for (size_t j = i + 1; j < patterns.size(); ++j) {
            // Skip patterns in different layers
            if (patterns[i].get_layer() != patterns[j].get_layer()) {
                continue;
            }
            
            if (!are_patterns_compatible(patterns[i], patterns[j])) {
                issues.emplace_back(
                    "Incompatible patterns",
                    patterns[i],
                    patterns[j]
                );
            }
        }
    }
    
    return issues;
}

std::vector<CompatibilityIssue> PatternCompatibilityManager::check_compatibility(
    const std::vector<const ArchitectureNode*>& nodes) const {
    
    std::vector<Pattern> patterns;
    for (const auto* node : nodes) {
        patterns.emplace_back(*node);
    }
    
    return check_compatibility(patterns);
}

bool PatternCompatibilityManager::are_patterns_compatible(
    const Pattern& p1, const Pattern& p2) const {
    
    // Patterns in different layers are always compatible
    if (p1.get_layer() != p2.get_layer()) {
        return true;
    }
    
    // Only one foundation pattern allowed
    if (p1.get_layer() == PatternLayer::FOUNDATION && p2.get_layer() == PatternLayer::FOUNDATION) {
        return false;
    }
    
    // Get the pattern names
    const std::string& name1 = p1.get_name();
    const std::string& name2 = p2.get_name();
    
    // Check if we have compatibility rules for these patterns
    auto it1 = m_compatibility_rules.find(name1);
    auto it2 = m_compatibility_rules.find(name2);
    
    // If we don't have rules for either pattern, assume they're compatible
    if (it1 == m_compatibility_rules.end() || it2 == m_compatibility_rules.end()) {
        return true;
    }
    
    // Check if p2 is in p1's incompatible patterns
    if (it1->second.incompatible_patterns.find(name2) != it1->second.incompatible_patterns.end()) {
        return false;
    }
    
    // Check if p1 is in p2's incompatible patterns
    if (it2->second.incompatible_patterns.find(name1) != it2->second.incompatible_patterns.end()) {
        return false;
    }
    
    // If p2 is in p1's compatible patterns or p1 is in p2's compatible patterns, they're compatible
    if (it1->second.compatible_patterns.find(name2) != it1->second.compatible_patterns.end() ||
        it2->second.compatible_patterns.find(name1) != it2->second.compatible_patterns.end()) {
        return true;
    }
    
    // If we have rules for both patterns but they don't explicitly allow or disallow each other,
    // we default to incompatible
    return false;
}

} // namespace cql