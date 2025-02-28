// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <memory>
#include "../../include/cql/pattern_compatibility.hpp"
#include "../../include/cql/nodes.hpp"
#include "../../include/cql/cql.hpp"

namespace cql::test {

TestResult test_architecture_patterns() {
    std::cout << "Testing architecture patterns..." << std::endl;
    
    try {
        // Test pattern layer string conversion
        TEST_ASSERT(pattern_layer_to_string(PatternLayer::FOUNDATION) == "foundation",
                   "FOUNDATION should convert to 'foundation'");
        TEST_ASSERT(pattern_layer_to_string(PatternLayer::COMPONENT) == "component",
                   "COMPONENT should convert to 'component'");
        TEST_ASSERT(pattern_layer_to_string(PatternLayer::INTERACTION) == "interaction",
                   "INTERACTION should convert to 'interaction'");
        
        // Test string to pattern layer conversion
        TEST_ASSERT(string_to_pattern_layer("foundation") == PatternLayer::FOUNDATION,
                   "'foundation' should convert to FOUNDATION");
        TEST_ASSERT(string_to_pattern_layer("FOUNDATION") == PatternLayer::FOUNDATION,
                   "'FOUNDATION' should convert to FOUNDATION");
        TEST_ASSERT(string_to_pattern_layer("component") == PatternLayer::COMPONENT,
                   "'component' should convert to COMPONENT");
        TEST_ASSERT(string_to_pattern_layer("interaction") == PatternLayer::INTERACTION,
                   "'interaction' should convert to INTERACTION");
        
        // Test default case
        TEST_ASSERT(string_to_pattern_layer("unknown") == PatternLayer::COMPONENT,
                   "'unknown' should default to COMPONENT");
        
        // Test pattern creation from nodes
        ArchitectureNode foundation_node(PatternLayer::FOUNDATION, "microservices", "");
        ArchitectureNode component_node1(PatternLayer::COMPONENT, "factory_method", "products: [\"Document\", \"Image\"]");
        ArchitectureNode component_node2(PatternLayer::COMPONENT, "singleton", "thread_safe: true");
        ArchitectureNode interaction_node(PatternLayer::INTERACTION, "observer", "events: [\"documentChanged\"]");
        
        // Create patterns from nodes
        Pattern foundation_pattern(foundation_node);
        Pattern component_pattern1(component_node1);
        Pattern component_pattern2(component_node2);
        Pattern interaction_pattern(interaction_node);
        
        // Verify pattern properties
        TEST_ASSERT(foundation_pattern.get_layer() == PatternLayer::FOUNDATION,
                   "Foundation pattern should have FOUNDATION layer");
        TEST_ASSERT(foundation_pattern.get_name() == "microservices",
                   "Foundation pattern name should be 'microservices'");
        
        TEST_ASSERT(component_pattern1.get_layer() == PatternLayer::COMPONENT,
                   "Component pattern should have COMPONENT layer");
        TEST_ASSERT(component_pattern1.get_name() == "factory_method",
                   "Component pattern name should be 'factory_method'");
        TEST_ASSERT(component_pattern1.get_parameters().find("products") != std::string::npos,
                   "Component pattern parameters should contain 'products'");
        
        TEST_ASSERT(interaction_pattern.get_layer() == PatternLayer::INTERACTION,
                   "Interaction pattern should have INTERACTION layer");
        TEST_ASSERT(interaction_pattern.get_name() == "observer",
                   "Interaction pattern name should be 'observer'");
        
        // Test pattern compatibility manager
        PatternCompatibilityManager manager;
        
        // Test compatible patterns
        TEST_ASSERT(manager.are_patterns_compatible(component_pattern1, component_pattern2) == true,
                   "Factory method and singleton should be compatible");
        
        // Test different layers (should always be compatible)
        TEST_ASSERT(manager.are_patterns_compatible(foundation_pattern, component_pattern1) == true,
                   "Patterns in different layers should be compatible");
        TEST_ASSERT(manager.are_patterns_compatible(interaction_pattern, component_pattern2) == true,
                   "Patterns in different layers should be compatible");
        
        // Test foundation uniqueness constraint
        ArchitectureNode foundation_node2(PatternLayer::FOUNDATION, "layered_architecture", "");
        Pattern foundation_pattern2(foundation_node2);
        
        std::vector<Pattern> patterns = {foundation_pattern, foundation_pattern2};
        auto issues = manager.check_compatibility(patterns);
        
        TEST_ASSERT(!issues.empty(),
                   "Multiple foundation patterns should generate compatibility issues");
        
        // Test incompatible patterns
        ArchitectureNode prototype_node(PatternLayer::COMPONENT, "prototype", "deep_copy: true");
        Pattern prototype_pattern(prototype_node);
        
        TEST_ASSERT(manager.are_patterns_compatible(component_pattern2, prototype_pattern) == false,
                   "Singleton and prototype should be incompatible");
        
        // Test legacy pattern format
        ArchitectureNode legacy_node("Singleton pattern with thread safety");
        Pattern legacy_pattern(legacy_node);
        
        TEST_ASSERT(legacy_pattern.get_layer() == PatternLayer::COMPONENT,
                   "Legacy pattern should default to COMPONENT layer");
        
        // Try using a compatibility issue's string representation
        if (!issues.empty()) {
            std::string issue_str = issues[0].to_string();
            TEST_ASSERT(!issue_str.empty(),
                       "Compatibility issue string representation should not be empty");
        }
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_architecture_patterns: " + 
                              std::string(e.what()), __FILE__, __LINE__);
    }
}

} // namespace cql::test