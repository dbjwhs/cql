#include <iostream>
#include <cassert>
#include <sstream>
#include <vector>
#include <map>
#include <filesystem>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_manager.hpp"
#include "../../../headers/project_utils.hpp"

namespace fs = std::filesystem;

namespace cql::test {

// Run all tests
void run_tests() {
    std::cout << "Running CQL Tests..." << std::endl;
    bool all_passed = true;
    
    try {
        // Test compiler functionalities
        test_basic_compilation();
        test_complex_compilation();
        
        // Test Phase 2 features
        test_phase2_features();
        
        // Test template management
        test_template_management();
        
        // Test template inheritance
        test_template_inheritance();
        
        // Test template validator
        test_template_validator();
        
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        all_passed = false;
    }
    
    if (!all_passed) {
        std::cerr << "Some tests failed!" << std::endl;
        return;
    }
}

// Test basic query compilation
void test_basic_compilation() {
    std::cout << "Testing basic compilation..." << std::endl;
    
    std::string query = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\"\n@description \"test compilation\"";
    std::string result = QueryProcessor::compile(query);
    
    assert(!result.empty());
    assert(result.find("MIT License") != std::string::npos);
    assert(result.find("Copyright (c) 2025 dbjwhs") != std::string::npos);
    assert(result.find("Language: C++") != std::string::npos);
    
    std::cout << "Basic compilation test passed." << std::endl;
}

// Test more complex query compilation
void test_complex_compilation() {
    std::cout << "Testing complex compilation..." << std::endl;
    
    std::string query = 
        "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
        "@language \"C++\"\n"
        "@description \"implement a thread-safe queue\"\n"
        "@context \"Using C++20 features\"\n"
        "@context \"Must be exception-safe\"\n"
        "@dependency \"std::mutex, std::condition_variable\"\n"
        "@test \"Test concurrent push operations\"\n"
        "@test \"Test concurrent pop operations\"\n";
    
    std::string result = QueryProcessor::compile(query);
    
    assert(!result.empty());
    assert(result.find("MIT License") != std::string::npos);
    assert(result.find("C++") != std::string::npos);
    assert(result.find("thread-safe queue") != std::string::npos);
    assert(result.find("C++20") != std::string::npos);
    assert(result.find("exception-safe") != std::string::npos);
    assert(result.find("Test concurrent push") != std::string::npos);
    assert(result.find("Test concurrent pop") != std::string::npos);
    
    std::cout << "Complex compilation test passed." << std::endl;
}

// Test the Phase 2 features
void test_phase2_features() {
    std::cout << "Testing Phase 2 features..." << std::endl;
    
    std::string query = 
        "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
        "@language \"C++\"\n"
        "@description \"implement a thread-safe queue with a maximum size\"\n"
        "@context \"Using C++20 features and RAII principles\"\n"
        "@architecture \"Producer-consumer pattern with monitoring\"\n"
        "@constraint \"Thread-safe for concurrent access\"\n"
        "@security \"Prevent data races and deadlocks\"\n"
        "@complexity \"O(1) for push and pop operations\"\n"
        "@variable \"max_size\" \"1000\"\n"
        "@example \"Basic Usage\" \"\n"
        "ThreadSafeQueue<int> queue(${max_size});\n"
        "queue.push(42);\n"
        "auto value = queue.pop();\n"
        "\"\n"
        "@test \"Test concurrent push operations\"\n"
        "@test \"Test concurrent pop operations\"\n"
        "@test \"Test boundary conditions\"\n";
    
    std::string result = QueryProcessor::compile(query);
    
    assert(!result.empty());
    assert(result.find("Architecture: Producer-consumer pattern with monitoring") != std::string::npos);
    assert(result.find("Constraints: Thread-safe for concurrent access") != std::string::npos);
    assert(result.find("Security: Prevent data races and deadlocks") != std::string::npos);
    assert(result.find("Complexity: O(1) for push and pop operations") != std::string::npos);
    assert(result.find("ThreadSafeQueue<int> queue(1000);") != std::string::npos);
    
    std::cout << "Phase 2 features test passed." << std::endl;
}

// Test template management
void test_template_management() {
    std::cout << "Testing template management..." << std::endl;
    
    // Create a temporary template directory for testing
    std::string temp_dir = "./temp_templates";
    
    try {
        // Create a temporary template directory
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directory(temp_dir);
        
        // Create a template manager with the temp directory
        TemplateManager manager(temp_dir);
        
        // Test saving a template
        std::string template_content = 
            "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
            "@description \"test template\"\n"
            "@variable \"test_var\" \"test_value\"\n"
            "@language \"${test_var}\"\n";
        
        manager.save_template("test_template", template_content);
        
        // Test listing templates
        auto templates = manager.list_templates();
        assert(templates.size() == 1);
        assert(templates[0].find("test_template") != std::string::npos);
        
        // Test loading a template
        std::string loaded = manager.load_template("test_template");
        assert(loaded == template_content);
        
        // Test getting template metadata
        auto metadata = manager.get_template_metadata("test_template");
        assert(metadata.name.find("test_template") != std::string::npos);
        assert(metadata.description == "test template");
        assert(metadata.variables.size() == 1);
        assert(metadata.variables[0] == "test_var");
        
        // Test template instantiation with variables
        std::map<std::string, std::string> vars = {{"test_var", "C++"}};
        std::string instantiated = manager.instantiate_template("test_template", vars);
        assert(instantiated.find("@language \"C++\"") != std::string::npos);
        
        // Test creating a category
        assert(manager.create_category("test_category"));
        
        // Test saving a template in a category
        manager.save_template("test_category/category_template", template_content);
        
        // Test listing categories
        auto categories = manager.list_categories();
        assert(categories.size() >= 3); // common, user, test_category
        bool found_category = false;
        for (const auto& cat : categories) {
            if (cat == "test_category") {
                found_category = true;
                break;
            }
        }
        assert(found_category);
        
        // Test deleting a template
        assert(manager.delete_template("test_template"));
        templates = manager.list_templates();
        bool template_found = false;
        for (const auto& tmpl : templates) {
            if (tmpl.find("test_template") != std::string::npos && 
                tmpl.find("test_category") == std::string::npos) {
                template_found = true;
                break;
            }
        }
        assert(!template_found);
        
        // Cleanup
        fs::remove_all(temp_dir);
        
        std::cout << "Template management test passed." << std::endl;
    } catch (const std::exception& e) {
        // Ensure cleanup even if test fails
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        throw;
    }
}

// Test template inheritance feature
void test_template_inheritance() {
    std::cout << "Testing template inheritance..." << std::endl;
    
    // Create a temporary template directory for testing
    std::string temp_dir = "./temp_templates";
    
    try {
        // Create a temporary template directory
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directory(temp_dir);
        
        // Create a template manager with the temp directory
        TemplateManager manager(temp_dir);
        
        // Create a base template
        std::string base_template_content = 
            "@description \"base template\"\n"
            "@variable \"base_var\" \"base_value\"\n"
            "@variable \"shared_var\" \"base_shared_value\"\n"
            "@test \"Base test\"\n";
        
        manager.save_template("base_template", base_template_content);
        
        // Create a child template inheriting from base
        std::string child_template_content = 
            "@inherit \"base_template\"\n"
            "@description \"child template\"\n"
            "@variable \"child_var\" \"child_value\"\n"
            "@variable \"shared_var\" \"child_shared_value\"\n" // Override shared_var
            "@test \"Child test\"\n";
        
        manager.save_template("child_template", child_template_content);
        
        // Create a grandchild template inheriting from child
        std::string grandchild_template_content = 
            "@inherit \"child_template\"\n"
            "@description \"grandchild template\"\n"
            "@variable \"grandchild_var\" \"grandchild_value\"\n"
            "@test \"Grandchild test\"\n";
        
        manager.save_template("grandchild_template", grandchild_template_content);
        
        // Test inheritance chain
        auto chain = manager.get_inheritance_chain("grandchild_template");
        assert(chain.size() == 3);
        assert(chain[0] == "base_template");
        assert(chain[1] == "child_template");
        assert(chain[2] == "grandchild_template");
        
        // Test metadata includes parent information
        auto metadata = manager.get_template_metadata("child_template");
        assert(metadata.parent.has_value());
        assert(metadata.parent.value() == "base_template");
        
        // Test template loading with inheritance
        std::string loaded = manager.load_template_with_inheritance("grandchild_template");
        
        // Verify variable merging and overriding
        assert(loaded.find("\"base_var\" \"base_value\"") != std::string::npos); // Base var preserved
        assert(loaded.find("\"child_var\" \"child_value\"") != std::string::npos); // Child var preserved
        assert(loaded.find("\"grandchild_var\" \"grandchild_value\"") != std::string::npos); // Grandchild var preserved
        assert(loaded.find("\"shared_var\" \"child_shared_value\"") != std::string::npos); // Child override preserved
        assert(loaded.find("\"shared_var\" \"base_shared_value\"") == std::string::npos); // Base override removed
        
        // Verify content merging
        assert(loaded.find("Base test") != std::string::npos); // Base test included
        assert(loaded.find("Child test") != std::string::npos); // Child test included
        assert(loaded.find("Grandchild test") != std::string::npos); // Grandchild test included
        
        // Test instantiation with inheritance
        std::map<std::string, std::string> vars = {
            {"base_var", "new_base_value"},
            {"child_var", "new_child_value"},
            {"grandchild_var", "new_grandchild_value"},
            {"shared_var", "new_shared_value"}
        };
        
        std::string instantiated = manager.instantiate_template("grandchild_template", vars);
        
        // Verify variable replacement with overrides
        assert(instantiated.find("\"base_var\" \"new_base_value\"") != std::string::npos);
        assert(instantiated.find("\"child_var\" \"new_child_value\"") != std::string::npos);
        assert(instantiated.find("\"grandchild_var\" \"new_grandchild_value\"") != std::string::npos);
        assert(instantiated.find("\"shared_var\" \"new_shared_value\"") != std::string::npos);
        
        // Test circular inheritance detection
        std::string circular_template_content = 
            "@inherit \"circular_template\"\n"
            "@description \"circular template\"\n";
        
        manager.save_template("circular_template", circular_template_content);
        
        try {
            // This should throw an exception due to circular inheritance
            manager.load_template_with_inheritance("circular_template");
            assert(false); // Should not reach here
        } catch (const std::exception& e) {
            // Exception is expected
            std::string error = e.what();
            assert(error.find("circular") != std::string::npos);
        }
        
        // Cleanup
        fs::remove_all(temp_dir);
        
        std::cout << "Template inheritance test passed." << std::endl;
    } catch (const std::exception& e) {
        // Ensure cleanup even if test fails
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        throw;
    }
}

// Show example queries for documentation
void query_examples() {
    std::cout << "\nCQL Query Examples:" << std::endl;
    
    // Define some example queries
    std::vector<std::pair<std::string, std::string>> examples = {
        {"Basic Copyright and Language", 
         "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\""},
         
        {"Thread-safe Queue", 
         "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
         "@language \"C++\"\n"
         "@description \"implement a thread-safe queue\"\n"
         "@context \"Using C++20 features\"\n"
         "@context \"Must be exception-safe\"\n"
         "@dependency \"std::mutex, std::condition_variable\"\n"
         "@test \"Test concurrent push operations\"\n"
         "@test \"Test concurrent pop operations\"\n"},
         
        {"Variable Example",
         "@variable \"container_type\" \"vector\"\n"
         "@variable \"element_type\" \"int\"\n"
         "@description \"implement a ${container_type}<${element_type}> class\"\n"
         "@language \"C++\"\n"
         "@test \"Test ${container_type} operations\"\n"}
    };
    
    // Process and display each example
    for (const auto& [title, query] : examples) {
        std::cout << "\n=== " << title << " ===\n" << std::endl;
        std::cout << "Query:\n" << query << std::endl;
        
        try {
            std::string result = QueryProcessor::compile(query);
            std::cout << "\nCompiled Result:\n" << result << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error compiling example: " << e.what() << std::endl;
        }
    }
}

// Test template validator
void test_template_validator() {
    std::cout << "Testing template validator..." << std::endl;
    
    // Create a temporary template directory for testing
    std::string temp_dir = "./temp_templates";
    
    try {
        // Create a temporary template directory
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directory(temp_dir);
        
        // Create a template manager with the temp directory
        TemplateManager manager(temp_dir);
        
        // Create a template validator
        TemplateValidator validator(manager);
        
        // Test 1: Template with all variables declared and used
        std::string good_template = 
            "@description \"A good template with proper variables\"\n"
            "@variable \"var1\" \"value1\"\n"
            "@variable \"var2\" \"value2\"\n"
            "@language \"${var1}\"\n"
            "@context \"Using ${var2} features\"\n";
        
        manager.save_template("good_template", good_template);
        
        auto good_result = validator.validate_template("good_template");
        assert(!good_result.has_issues(TemplateValidationLevel::ERROR));
        assert(!good_result.has_issues(TemplateValidationLevel::WARNING));
        
        // Test 2: Template with undeclared variable (should generate warning)
        std::string warning_template = 
            "@description \"A template with undeclared variable\"\n"
            "@variable \"var1\" \"value1\"\n"
            "@language \"${var1}\"\n"
            "@context \"Using ${undeclared_var} features\"\n";
        
        manager.save_template("warning_template", warning_template);
        
        auto warning_result = validator.validate_template("warning_template");
        assert(!warning_result.has_issues(TemplateValidationLevel::ERROR));
        assert(warning_result.has_issues(TemplateValidationLevel::WARNING));
        assert(warning_result.count_warnings() > 0);
        
        // Test 3: Template with unused variable (should generate info)
        std::string info_template = 
            "@description \"A template with unused variable\"\n"
            "@variable \"var1\" \"value1\"\n"
            "@variable \"unused_var\" \"unused_value\"\n"
            "@language \"${var1}\"\n";
        
        manager.save_template("info_template", info_template);
        
        auto info_result = validator.validate_template("info_template");
        assert(!info_result.has_issues(TemplateValidationLevel::ERROR));
        assert(info_result.has_issues(TemplateValidationLevel::INFO));
        assert(info_result.count_infos() > 0);
        
        // Test 4: Template with circular inheritance (should generate error)
        std::string circular1 = "@description \"Template with circular inheritance\"\n@inherit \"circular2\"\n";
        std::string circular2 = "@description \"Another template in the circle\"\n@inherit \"circular1\"\n";
        
        manager.save_template("circular1", circular1);
        manager.save_template("circular2", circular2);
        
        auto circular_result = validator.validate_template("circular1");
        assert(circular_result.has_issues(TemplateValidationLevel::ERROR));
        assert(circular_result.count_errors() > 0);
        
        // Test 5: Template with proper inheritance
        std::string parent = 
            "@description \"Parent template\"\n"
            "@variable \"parent_var\" \"parent_value\"\n"
            "@language \"${parent_var}\"\n";
        
        std::string child = 
            "@inherit \"parent\"\n"
            "@description \"Child template\"\n"
            "@variable \"child_var\" \"child_value\"\n"
            "@context \"${child_var} with ${parent_var}\"\n";
        
        manager.save_template("parent", parent);
        manager.save_template("child", child);
        
        auto inheritance_result = validator.validate_template("child");
        assert(!inheritance_result.has_issues(TemplateValidationLevel::ERROR));
        
        // Cleanup
        fs::remove_all(temp_dir);
        
        std::cout << "Template validator test passed." << std::endl;
    } catch (const std::exception& e) {
        // Ensure cleanup even if test fails
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        throw;
    }
}

} // namespace cql::test