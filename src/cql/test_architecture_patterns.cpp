// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <memory>
#include <gtest/gtest.h>
#include "../../include/cql/pattern_compatibility.hpp"
#include "../../include/cql/nodes.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/test_utils.hpp"

namespace cql::test {

// Google Test fixture for architecture pattern tests
class ArchitecturePatternsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code common to all tests
    }

    void TearDown() override {
        // Cleanup code common to all tests
    }

    // Helper method to create test patterns
    void CreateTestPatterns() {
        foundation_node = std::make_unique<ArchitectureNode>(PatternLayer::FOUNDATION, "microservices", "");
        component_node1 = std::make_unique<ArchitectureNode>(PatternLayer::COMPONENT, "factory_method", R"(products: ["Document", "Image"])");
        component_node2 = std::make_unique<ArchitectureNode>(PatternLayer::COMPONENT, "singleton", "thread_safe: true");
        interaction_node = std::make_unique<ArchitectureNode>(PatternLayer::INTERACTION, "observer", "events: [\"documentChanged\"]");
        
        foundation_pattern = std::make_unique<Pattern>(*foundation_node);
        component_pattern1 = std::make_unique<Pattern>(*component_node1);
        component_pattern2 = std::make_unique<Pattern>(*component_node2);
        interaction_pattern = std::make_unique<Pattern>(*interaction_node);
    }

    // Test pattern nodes
    std::unique_ptr<ArchitectureNode> foundation_node;
    std::unique_ptr<ArchitectureNode> component_node1;
    std::unique_ptr<ArchitectureNode> component_node2;
    std::unique_ptr<ArchitectureNode> interaction_node;
    
    // Test patterns
    std::unique_ptr<Pattern> foundation_pattern;
    std::unique_ptr<Pattern> component_pattern1;
    std::unique_ptr<Pattern> component_pattern2;
    std::unique_ptr<Pattern> interaction_pattern;
    
    // Pattern compatibility manager
    PatternCompatibilityManager manager;
};

// Test pattern layer enum conversions
TEST_F(ArchitecturePatternsTest, PatternLayerConversion) {
    // Test pattern layer string conversion
    ASSERT_EQ(pattern_layer_to_string(PatternLayer::FOUNDATION), "foundation") 
        << "FOUNDATION should convert to 'foundation'";
    ASSERT_EQ(pattern_layer_to_string(PatternLayer::COMPONENT), "component")
        << "COMPONENT should convert to 'component'";
    ASSERT_EQ(pattern_layer_to_string(PatternLayer::INTERACTION), "interaction")
        << "INTERACTION should convert to 'interaction'";
    
    // Test string to pattern layer conversion
    ASSERT_EQ(string_to_pattern_layer("foundation"), PatternLayer::FOUNDATION)
        << "'foundation' should convert to FOUNDATION";
    ASSERT_EQ(string_to_pattern_layer("FOUNDATION"), PatternLayer::FOUNDATION)
        << "'FOUNDATION' should convert to FOUNDATION";
    ASSERT_EQ(string_to_pattern_layer("component"), PatternLayer::COMPONENT)
        << "'component' should convert to COMPONENT";
    ASSERT_EQ(string_to_pattern_layer("interaction"), PatternLayer::INTERACTION)
        << "'interaction' should convert to INTERACTION";
    
    // Test default case
    ASSERT_EQ(string_to_pattern_layer("unknown"), PatternLayer::COMPONENT)
        << "'unknown' should default to COMPONENT";
}

// Test pattern creation from nodes
TEST_F(ArchitecturePatternsTest, PatternCreation) {
    CreateTestPatterns();
    
    // Verify pattern properties
    ASSERT_EQ(foundation_pattern->get_layer(), PatternLayer::FOUNDATION)
        << "Foundation pattern should have FOUNDATION layer";
    ASSERT_EQ(foundation_pattern->get_name(), "microservices")
        << "Foundation pattern name should be 'microservices'";
    
    ASSERT_EQ(component_pattern1->get_layer(), PatternLayer::COMPONENT)
        << "Component pattern should have COMPONENT layer";
    ASSERT_EQ(component_pattern1->get_name(), "factory_method")
        << "Component pattern name should be 'factory_method'";
    ASSERT_NE(component_pattern1->get_parameters().find("products"), std::string::npos)
        << "Component pattern parameters should contain 'products'";
    
    ASSERT_EQ(interaction_pattern->get_layer(), PatternLayer::INTERACTION)
        << "Interaction pattern should have INTERACTION layer";
    ASSERT_EQ(interaction_pattern->get_name(), "observer")
        << "Interaction pattern name should be 'observer'";
}

// Test pattern compatibility
TEST_F(ArchitecturePatternsTest, PatternCompatibility) {
    CreateTestPatterns();
    
    // Test compatible patterns
    ASSERT_TRUE(manager.are_patterns_compatible(*component_pattern1, *component_pattern2))
        << "Factory method and singleton should be compatible";
    
    // Test different layers (should always be compatible)
    ASSERT_TRUE(manager.are_patterns_compatible(*foundation_pattern, *component_pattern1))
        << "Patterns in different layers should be compatible";
    ASSERT_TRUE(manager.are_patterns_compatible(*interaction_pattern, *component_pattern2))
        << "Patterns in different layers should be compatible";
}

// Test foundation uniqueness constraint
TEST_F(ArchitecturePatternsTest, FoundationUniqueness) {
    CreateTestPatterns();

    const ArchitectureNode foundation_node2(PatternLayer::FOUNDATION, "layered_architecture", "");
    const Pattern foundation_pattern2(foundation_node2);

    const std::vector<Pattern> patterns = {*foundation_pattern, foundation_pattern2};
    auto issues = manager.check_compatibility(patterns);
    
    ASSERT_FALSE(issues.empty())
        << "Multiple foundation patterns should generate compatibility issues";
}

// Test incompatible patterns
TEST_F(ArchitecturePatternsTest, IncompatiblePatterns) {
    CreateTestPatterns();

    const ArchitectureNode prototype_node(PatternLayer::COMPONENT, "prototype", "deep_copy: true");
    const Pattern prototype_pattern(prototype_node);
    
    ASSERT_FALSE(manager.are_patterns_compatible(*component_pattern2, prototype_pattern))
        << "Singleton and prototype should be incompatible";
}

// Test structural patterns
TEST_F(ArchitecturePatternsTest, StructuralPatterns) {
    ArchitectureNode bridge_node(PatternLayer::COMPONENT, "bridge", 
                                R"(implementors: ["WindowsRenderer", "MacOSRenderer"])");
    ArchitectureNode composite_node(PatternLayer::COMPONENT, "composite", 
                                  "component_type: \"UIComponent\"");
    ArchitectureNode decorator_node(PatternLayer::COMPONENT, "decorator", 
                                  R"(decorations: ["Border", "Shadow"])");
    
    Pattern bridge_pattern(bridge_node);
    Pattern composite_pattern(composite_node);
    Pattern decorator_pattern(decorator_node);
    
    // Test bridge and composite incompatibility
    ASSERT_FALSE(manager.are_patterns_compatible(bridge_pattern, composite_pattern))
        << "Bridge and composite should be incompatible";
    
    // Test decorator compatibility
    ASSERT_TRUE(manager.are_patterns_compatible(decorator_pattern, composite_pattern))
        << "Decorator and composite should be compatible";
}

// Test multiple patterns compatibility
TEST_F(ArchitecturePatternsTest, MultiplePatternCompatibility) {
    CreateTestPatterns();
    
    ArchitectureNode composite_node(PatternLayer::COMPONENT, "composite", 
                                  "component_type: \"UIComponent\"");
    ArchitectureNode decorator_node(PatternLayer::COMPONENT, "decorator", 
                                  R"(decorations: ["Border", "Shadow"])");
    
    Pattern composite_pattern(composite_node);
    Pattern decorator_pattern(decorator_node);
    
    // Test multiple patterns compatibility
    std::vector<Pattern> ui_patterns = {
        composite_pattern, decorator_pattern, *component_pattern1 // factory + composite + decorator
    };
    auto ui_issues = manager.check_compatibility(ui_patterns);
    ASSERT_TRUE(ui_issues.empty())
        << "Factory + Composite + Decorator should be compatible";
}

// Test behavioral patterns
TEST_F(ArchitecturePatternsTest, BehavioralPatterns) {
    ArchitectureNode observer_node(PatternLayer::INTERACTION, "observer", 
                                R"(events: ["valueChanged", "objectCreated"])");
    ArchitectureNode command_node(PatternLayer::INTERACTION, "command", 
                                R"(commands: ["SaveCommand", "DeleteCommand"])");
    ArchitectureNode strategy_node(PatternLayer::INTERACTION, "strategy", 
                                  R"(strategies: ["FastStrategy", "AccurateStrategy"])");
    
    Pattern observer_pattern(observer_node);
    Pattern command_pattern(command_node);
    Pattern strategy_pattern(strategy_node);
    
    // Test behavioral patterns compatibility
    ASSERT_TRUE(manager.are_patterns_compatible(command_pattern, observer_pattern))
        << "Command and Observer should be compatible";
    ASSERT_TRUE(manager.are_patterns_compatible(command_pattern, strategy_pattern))
        << "Command and Strategy should be compatible";
}

// Test combined pattern types
TEST_F(ArchitecturePatternsTest, CombinedPatternTypes) {
    CreateTestPatterns();
    
    ArchitectureNode decorator_node(PatternLayer::COMPONENT, "decorator", 
                                  R"(decorations: ["Border", "Shadow"])");
    ArchitectureNode observer_node(PatternLayer::INTERACTION, "observer", 
                                R"(events: ["valueChanged", "objectCreated"])");
    
    Pattern decorator_pattern(decorator_node);
    Pattern observer_pattern(observer_node);
    
    // Test combined creational, structural, and behavioral patterns
    std::vector<Pattern> complex_patterns = {
        *component_pattern1,  // factory_method - creational
        decorator_pattern,    // decorator - structural
        observer_pattern      // observer - behavioral
    };
    
    auto complex_issues = manager.check_compatibility(complex_patterns);
    ASSERT_TRUE(complex_issues.empty())
        << "Factory + Decorator + Observer should be compatible";
}

// Test legacy pattern format
TEST_F(ArchitecturePatternsTest, LegacyPatternFormat) {
    ArchitectureNode legacy_node("Singleton pattern with thread safety");
    Pattern legacy_pattern(legacy_node);
    
    ASSERT_EQ(legacy_pattern.get_layer(), PatternLayer::COMPONENT)
        << "Legacy pattern should default to COMPONENT layer";
}

// Test compatibility issues string representation
TEST_F(ArchitecturePatternsTest, CompatibilityIssueString) {
    CreateTestPatterns();
    
    ArchitectureNode foundation_node2(PatternLayer::FOUNDATION, "layered_architecture", "");
    Pattern foundation_pattern2(foundation_node2);
    
    std::vector<Pattern> patterns = {*foundation_pattern, foundation_pattern2};
    auto issues = manager.check_compatibility(patterns);
    
    ASSERT_FALSE(issues.empty())
        << "Multiple foundation patterns should generate compatibility issues";
    
    std::string issue_str = issues[0].to_string();
    ASSERT_FALSE(issue_str.empty())
        << "Compatibility issue string representation should not be empty";
}

// Legacy function for backward compatibility
TestResult test_architecture_patterns() {
    try {
        // Test pattern layer string conversion
        if (pattern_layer_to_string(PatternLayer::FOUNDATION) != "foundation") {
            return TestResult::fail("FOUNDATION should convert to 'foundation'", __FILE__, __LINE__);
        }
        if (pattern_layer_to_string(PatternLayer::COMPONENT) != "component") {
            return TestResult::fail("COMPONENT should convert to 'component'", __FILE__, __LINE__);
        }
        if (pattern_layer_to_string(PatternLayer::INTERACTION) != "interaction") {
            return TestResult::fail("INTERACTION should convert to 'interaction'", __FILE__, __LINE__);
        }
        
        // Test string to pattern layer conversion
        if (string_to_pattern_layer("foundation") != PatternLayer::FOUNDATION) {
            return TestResult::fail("'foundation' should convert to FOUNDATION", __FILE__, __LINE__);
        }
        if (string_to_pattern_layer("FOUNDATION") != PatternLayer::FOUNDATION) {
            return TestResult::fail("'FOUNDATION' should convert to FOUNDATION", __FILE__, __LINE__);
        }
        if (string_to_pattern_layer("component") != PatternLayer::COMPONENT) {
            return TestResult::fail("'component' should convert to COMPONENT", __FILE__, __LINE__);
        }
        if (string_to_pattern_layer("interaction") != PatternLayer::INTERACTION) {
            return TestResult::fail("'interaction' should convert to INTERACTION", __FILE__, __LINE__);
        }
        if (string_to_pattern_layer("unknown") != PatternLayer::COMPONENT) {
            return TestResult::fail("'unknown' should default to COMPONENT", __FILE__, __LINE__);
        }
        
        // We're not testing all the patterns here since the Google Test version will handle that
        // do a basic check to make sure the legacy function passes
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_architecture_patterns: " + 
                              std::string(e.what()), __FILE__, __LINE__);
    }
}

} // namespace cql::test