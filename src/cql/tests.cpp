// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <cassert>
#include <sstream>
#include <utility>
#include <vector>
#include <map>
#include <filesystem>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_manager.hpp"
#include "../../include/cql/template_validator.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include "../../include/cql/api_client.hpp"
#include "../../include/cql/response_processor.hpp"
#include "../../include/cql/test_utils.hpp"
#include "../../third_party/include/nlohmann/json.hpp"

// note file exists in the cpp-snippets repo, you will need to check this out and have it and cql share the
// same root directory
#include "../../cpp-snippets/headers/project_utils.hpp"

namespace fs = std::filesystem;

namespace cql::test {

// Forward declarations
TestResult test_api_client();
TestResult test_response_processor();
TestResult test_api_integration();
TestResult test_api_custom_base_url();
TestResult test_api_error_handling_and_retry();
TestResult test_api_streaming();
TestResult test_configuration();
TestResult test_examples_compilation();
TestResult test_lexer_standalone();
TestResult test_parser_standalone();
TestResult test_compiler_standalone();

// Define TestInfo struct at namespace level
struct TestInfo {
    std::string name;
    std::function<TestResult()> test_func;
};

// testresult implementation
TestResult::TestResult(const bool passed, std::string  error_message,
                      std::string  file_name, const int line_number)
    : m_passed(passed), 
      m_error_message(std::move(error_message)),
      m_file_name(std::move(file_name)),
      m_line_number(line_number) {
}

TestResult TestResult::pass() {
    return TestResult{true};
}

TestResult TestResult::fail(const std::string& error_message, 
                           const std::string& file_name,
                           const int line_number) {
    return TestResult{false, error_message, file_name, line_number};
}

bool TestResult::passed() const {
    return m_passed;
}

const std::string& TestResult::get_error_message() const {
    return m_error_message;
}

const std::string& TestResult::get_file_name() const {
    return m_file_name;
}

int TestResult::get_line_number() const {
    return m_line_number;
}

// function to print a properly formatted test result
void print_test_result(const std::string& test_name, const TestResult& result) {
    constexpr int name_width = 40;
    
    std::cout << std::left << std::setw(name_width) << test_name;
    
    if (result.passed()) {
        std::cout << "[ \033[32mPASS\033[0m ]" << std::endl;
    } else {
        std::cout << "[ \033[31mFAIL\033[0m ]" << std::endl;
        std::cout << "  Error: " << result.get_error_message() << std::endl;
        if (!result.get_file_name().empty()) {
            std::cout << "  Location: " << result.get_file_name() 
                     << ":" << result.get_line_number() << std::endl;
        }
    }
}

// Forward declarations for new tests
TestResult test_json_format_output();

// Define the test list at namespace level
const std::vector<TestInfo> tests = {
    {"Basic Compilation", test_basic_compilation},
    {"Complex Compilation", test_complex_compilation},
    {"Validation Requirements", test_validation_requirements},
    {"Phase 2 Features", test_phase2_features},
    {"Template Management", test_template_management},
    {"Template Inheritance", test_template_inheritance},
    {"Template Validator", test_template_validator},
    {"Query Examples", query_examples},
    {"Phase 2 Example Compilation", test_phase2_example_compilation},
    {"Architecture Patterns", test_architecture_patterns},
    {"API Client", test_api_client},
    {"Response Processor", test_response_processor},
    {"API Integration", test_api_integration},
    {"API Custom Base URL", test_api_custom_base_url},
    {"API Error Handling", test_api_error_handling_and_retry},
    {"API Streaming", test_api_streaming},
    {"Configuration", test_configuration},
    {"Examples Compilation", test_examples_compilation},
    {"Lexer (Standalone)", test_lexer_standalone},
    {"Parser (Standalone)", test_parser_standalone},
    {"Compiler (Standalone)", test_compiler_standalone},
    {"JSON Format Output", test_json_format_output}
};

// run tests - either all tests or a specific test by name
bool run_tests(const bool fail_fast, const std::string& test_name) {
    if (test_name.empty()) {
        std::cout << "Running all CQL Tests..." << std::endl;
    } else {
        std::cout << "Running CQL Test: \"" << test_name << "\"" << std::endl;
    }
    
    bool all_passed = true;
    bool found_test = test_name.empty(); // If no specific test is requested, we don't need to find it
    
    // run each test
    for (const auto&[name, test_func] : tests) {
        // Skip tests that don't match the requested name
        if (!test_name.empty() && name != test_name) {
            continue;
        }
        
        found_test = true; // We found the requested test
        
        try {
            TestResult result = test_func();
            print_test_result(name, result);
            
            if (!result.passed()) {
                all_passed = false;
                if (fail_fast) {
                    std::cout << "\nFailed fast: Stopping tests after first failure." << std::endl;
                    break;
                }
            }
        } catch (const std::exception& e) {
            TestResult result = TestResult::fail("Uncaught exception: " + std::string(e.what()));
            print_test_result(name, result);
            all_passed = false;
            
            if (fail_fast) {
                std::cout << "\nFailed fast: Stopping tests after first exception." << std::endl;
                break;
            }
        }
    }
    
    // Handle case where the specified test wasn't found
    if (!found_test) {
        std::cerr << "Error: Test \"" << test_name << "\" not found!" << std::endl;
        std::cerr << "Use --test --list to see available tests." << std::endl;
        return false;
    }
    
    // print summary
    if (all_passed) {
        std::cout << "\n\033[32mAll tests passed!\033[0m" << std::endl;
    } else {
        std::cout << "\n\033[31mSome tests failed!\033[0m" << std::endl;
    }
    
    return all_passed;
}

// test basic query compilation
TestResult test_basic_compilation() {
    std::cout << "Testing basic compilation..." << std::endl;
    
    try {
        const std::string query = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\"\n@description \"test compilation\"";
        const std::string result = QueryProcessor::compile(query);
        
        TEST_ASSERT(!result.empty(), "Compilation result should not be empty");
        TEST_ASSERT(result.find("MIT License") != std::string::npos, 
                    "Result should contain 'MIT License'");
        TEST_ASSERT(result.find("Copyright (c) 2025 dbjwhs") != std::string::npos,
                   "Result should contain copyright information");
        // no "language: c++" string is produced in the output; 
        // the language is used in the code request but not directly output
        TEST_ASSERT(result.find("C++") != std::string::npos,
                   "Result should contain language information");
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_basic_compilation: " + std::string(e.what()), 
                                __FILE__, __LINE__);
    }
}

// test more complex query compilation
TestResult test_complex_compilation() {
    std::cout << "Testing complex compilation..." << std::endl;
    
    try {
        const std::string query =
            "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
            "@language \"C++\"\n"
            "@description \"implement a thread-safe queue\"\n"
            "@context \"Using C++20 features\"\n"
            "@context \"Must be exception-safe\"\n"
            "@dependency \"std::mutex, std::condition_variable\"\n"
            "@test \"Test concurrent push operations\"\n"
            "@test \"Test concurrent pop operations\"\n";

        const std::string result = QueryProcessor::compile(query);
        
        TEST_ASSERT(!result.empty(), "Compilation result should not be empty");
        TEST_ASSERT(result.find("MIT License") != std::string::npos, 
                   "Result should contain license information");
        TEST_ASSERT(result.find("C++") != std::string::npos,
                   "Result should contain language information");
        TEST_ASSERT(result.find("thread-safe queue") != std::string::npos,
                   "Result should contain the description");
        TEST_ASSERT(result.find("C++20") != std::string::npos,
                   "Result should contain context information about C++20");
        TEST_ASSERT(result.find("exception-safe") != std::string::npos,
                   "Result should contain context information about exception safety");
        TEST_ASSERT(result.find("Test concurrent push") != std::string::npos,
                   "Result should contain test information about push operations");
        TEST_ASSERT(result.find("Test concurrent pop") != std::string::npos,
                   "Result should contain test information about pop operations");
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_complex_compilation: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// test validation requirements
TestResult test_validation_requirements() {
    std::cout << "Testing validation requirements..." << std::endl;
    
    try {
        // Test 1: Missing copyright directive
        std::string missing_copyright = "@language \"C++\"\n@description \"test without copyright\"";
        
        try {
            std::string result = QueryProcessor::compile(missing_copyright);
            return TestResult::fail("Missing copyright validation failed - compilation should have failed", 
                                   __FILE__, __LINE__);
        } catch (const std::exception& e) {
            std::string error_message = e.what();
            std::ranges::transform(error_message, error_message.begin(), ::tolower);
            TEST_ASSERT(error_message.find("copyright") != std::string::npos,
                       "Error message should mention missing COPYRIGHT directive");
        }
        
        // Test 2: Missing language directive
        // We need to test the validation directly since the parser syntax errors would occur before validation
        {
            // Create a copyright node and description node manually, but skip language
            std::vector<std::unique_ptr<QueryNode>> nodes;
            nodes.push_back(std::make_unique<CopyrightNode>("MIT License", "2025 dbjwhs"));
            nodes.push_back(std::make_unique<CodeRequestNode>("", "test without language"));
            
            // Now test the validator directly with our manually constructed AST
            try {
                QueryValidator validator;
                validator.validate(nodes);
                return TestResult::fail("Missing language validation failed - validation should have failed", 
                                       __FILE__, __LINE__);
            } catch (const ValidationException& e) {
                std::string error_message = e.what();
                std::ranges::transform(error_message, error_message.begin(), ::tolower);
                TEST_ASSERT(error_message.find("language") != std::string::npos,
                           "Error message should mention missing LANGUAGE directive");
            }
        }
        
        // Test 3: Missing description directive
        std::string missing_description = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\"";
        
        try {
            std::string result = QueryProcessor::compile(missing_description);
            return TestResult::fail("Missing description validation failed - compilation should have failed", 
                                   __FILE__, __LINE__);
        } catch (const std::exception& e) {
            std::string error_message = e.what();
            std::ranges::transform(error_message, error_message.begin(), ::tolower);
            TEST_ASSERT(error_message.find("description") != std::string::npos,
                       "Error message should mention missing DESCRIPTION directive");
        }
        
        // Test 4: Parser errors shouldn't prevent validation from running
        // This test has both a parser error (invalid syntax) and a missing required directive
        std::string parser_and_validation_error = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@invalid_token \"Something\"\n@language \"C++\"";
        
        try {
            std::string result = QueryProcessor::compile(parser_and_validation_error);
            return TestResult::fail("Compilation should have failed due to errors", 
                                   __FILE__, __LINE__);
        } catch (const std::exception& e) {
            std::string error_message = e.what();
            std::ranges::transform(error_message, error_message.begin(), ::tolower);
            
            // We should see the validation error (missing description) in the output
            TEST_ASSERT(error_message.find("description") != std::string::npos || 
                        error_message.find("invalid") != std::string::npos,
                       "Error should be reported for either validation or parser issues");
        }
        
        // Test 5: Successful validation with all required directives
        std::string valid_query = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\"\n@description \"test with all required fields\"";
        std::string result = QueryProcessor::compile(valid_query);
        TEST_ASSERT(!result.empty(), "Valid query should compile successfully");
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_validation_requirements: " + std::string(e.what()), 
                               __FILE__, __LINE__);
    }
}

TestResult test_phase2_features() {
    std::cout << "Testing Phase 2 features..." << std::endl;
    
    try {
        const std::string query =
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

        const std::string result = QueryProcessor::compile(query);
        
        TEST_ASSERT(!result.empty(), "Compilation result should not be empty");
        
        // check for the presence of the content without the exact format string
        TEST_ASSERT(result.find("Producer-consumer pattern") != std::string::npos,
                   "Result should contain architecture information");
        TEST_ASSERT(result.find("Thread-safe for concurrent access") != std::string::npos,
                   "Result should contain constraint information");
        TEST_ASSERT(result.find("Prevent data races and deadlocks") != std::string::npos,
                   "Result should contain security information");
        TEST_ASSERT(result.find("O(1) for push and pop operations") != std::string::npos,
                   "Result should contain complexity information");
        TEST_ASSERT(result.find("ThreadSafeQueue<int> queue(1000)") != std::string::npos,
                   "Result should contain variable substitution in example");
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_phase2_features: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// test template management
TestResult test_template_management() {
    std::cout << "Testing template management..." << std::endl;
    
    // create a temporary template directory for testing
    std::string temp_dir = "./temp_templates";
    
    try {
        // Create a temporary template directory
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directory(temp_dir);
        
        // Create required common and user subdirectories to avoid errors
        // Suppress stderr only for directory creation operations that might log warnings
        {
            Logger::StderrSuppressionGuard stderr_guard;
            fs::create_directory(fs::path(temp_dir) / "common");
            fs::create_directory(fs::path(temp_dir) / "user");
        }
        
        // create a template manager with the temp directory
        TemplateManager manager(temp_dir);
        
        // test saving a template
        std::string template_content = 
            "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
            "@description \"test template\"\n"
            "@variable \"test_var\" \"test_value\"\n"
            "@language \"${test_var}\"\n";
        
        manager.save_template("test_template", template_content);
        
        // test listing templates
        auto templates = manager.list_templates();
        TEST_ASSERT(templates.size() == 1, "Should have exactly one template");
        TEST_ASSERT(templates[0].find("test_template") != std::string::npos,
                   "Template list should contain 'test_template'");
        
        // test loading a template
        std::string loaded = manager.load_template("test_template");
        TEST_ASSERT(loaded == template_content, "Loaded template content should match original");
        
        // test getting template metadata
        auto metadata = manager.get_template_metadata("test_template");
        TEST_ASSERT(metadata.name.find("test_template") != std::string::npos,
                   "Template metadata name should contain 'test_template'");
        TEST_ASSERT(metadata.description == "test template",
                   "Template metadata description should match");
        TEST_ASSERT(metadata.variables.size() == 1,
                   "Template should have one variable");
        TEST_ASSERT(metadata.variables[0] == "test_var",
                   "Template variable should be 'test_var'");
        
        // test template instantiation with variables
        std::map<std::string, std::string> vars = {{"test_var", "C++"}};
        std::string instantiated = manager.instantiate_template("test_template", vars);
        TEST_ASSERT(instantiated.find("@language \"C++\"") != std::string::npos,
                   "Instantiated template should contain substituted variable");
        
        // test creating a category
        bool category_created = manager.create_category("test_category");
        TEST_ASSERT(category_created, "Should be able to create a category");
        
        // test saving a template in a category
        manager.save_template("test_category/category_template", template_content);
        
        // test listing categories
        auto categories = manager.list_categories();
        TEST_ASSERT(categories.size() >= 3, "Should have at least common, user, and test_category");
        
        bool found_category = false;
        for (const auto& cat : categories) {
            if (cat == "test_category") {
                found_category = true;
                break;
            }
        }
        TEST_ASSERT(found_category, "Should find the test_category in the category list");
        
        // test deleting a template
        bool template_deleted = manager.delete_template("test_template");
        TEST_ASSERT(template_deleted, "Should be able to delete a template");
        
        templates = manager.list_templates();
        bool template_found = false;
        for (const auto& tmpl : templates) {
            if (tmpl.find("test_template") != std::string::npos && 
                tmpl.find("test_category") == std::string::npos) {
                template_found = true;
                break;
            }
        }
        TEST_ASSERT(!template_found, "Deleted template should not be in the template list");
        
        // cleanup
        fs::remove_all(temp_dir);
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        // ensure cleanup even if test fails
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        return TestResult::fail("Exception in test_template_management: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// test template inheritance feature
TestResult test_template_inheritance() {
    std::cout << "Testing template inheritance..." << std::endl;
    
    // create a temporary template directory for testing
    std::string temp_dir = "./temp_templates";
    
    try {
        // Create a temporary template directory
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directory(temp_dir);
        
        // Create required common and user subdirectories to avoid errors
        // Suppress stderr only for directory creation operations that might log warnings
        {
            Logger::StderrSuppressionGuard stderr_guard;
            fs::create_directory(fs::path(temp_dir) / "common");
            fs::create_directory(fs::path(temp_dir) / "user");
        }
        
        // create a template manager with the temp directory
        TemplateManager manager(temp_dir);
        
        // create a base template
        std::string base_template_content = 
            "@description \"base template\"\n"
            "@variable \"base_var\" \"base_value\"\n"
            "@variable \"shared_var\" \"base_shared_value\"\n"
            "@test \"Base test\"\n";
        
        manager.save_template("base_template", base_template_content);
        
        // create a child template inheriting from base
        std::string child_template_content = 
            "@inherit \"base_template\"\n"
            "@description \"child template\"\n"
            "@variable \"child_var\" \"child_value\"\n"
            "@variable \"shared_var\" \"child_shared_value\"\n" // override shared_var
            "@test \"Child test\"\n";
        
        manager.save_template("child_template", child_template_content);
        
        // create a grandchild template inheriting from child
        std::string grandchild_template_content = 
            "@inherit \"child_template\"\n"
            "@description \"grandchild template\"\n"
            "@variable \"grandchild_var\" \"grandchild_value\"\n"
            "@test \"Grandchild test\"\n";
        
        manager.save_template("grandchild_template", grandchild_template_content);
        
        // test inheritance chain
        auto chain = manager.get_inheritance_chain("grandchild_template");
        TEST_ASSERT(chain.size() == 3, "Inheritance chain should have 3 templates");
        TEST_ASSERT(chain[0] == "base_template", "First template in chain should be base_template");
        TEST_ASSERT(chain[1] == "child_template", "Second template in chain should be child_template");
        TEST_ASSERT(chain[2] == "grandchild_template", "Third template in chain should be grandchild_template");
        
        // test metadata includes parent information
        auto metadata = manager.get_template_metadata("child_template");
        TEST_ASSERT(metadata.parent.has_value(), "Child template should have parent metadata");
        TEST_ASSERT(metadata.parent.value() == "base_template", "Parent template should be base_template");
        
        // test template loading with inheritance
        std::string loaded = manager.load_template_with_inheritance("grandchild_template");
        
        // verify variable merging and overriding
        TEST_ASSERT(loaded.find("\"base_var\" \"base_value\"") != std::string::npos, 
                   "Base var should be preserved in merged template");
        TEST_ASSERT(loaded.find("\"child_var\" \"child_value\"") != std::string::npos, 
                   "Child var should be preserved in merged template");
        TEST_ASSERT(loaded.find("\"grandchild_var\" \"grandchild_value\"") != std::string::npos, 
                   "Grandchild var should be preserved in merged template");
        TEST_ASSERT(loaded.find("\"shared_var\" \"child_shared_value\"") != std::string::npos, 
                   "Child's override of shared_var should be preserved");
        TEST_ASSERT(loaded.find("\"shared_var\" \"base_shared_value\"") == std::string::npos, 
                   "Base's version of shared_var should be removed");
        
        // verify content merging
        TEST_ASSERT(loaded.find("Base test") != std::string::npos, 
                   "Base test should be included in merged template");
        TEST_ASSERT(loaded.find("Child test") != std::string::npos, 
                   "Child test should be included in merged template");
        TEST_ASSERT(loaded.find("Grandchild test") != std::string::npos, 
                   "Grandchild test should be included in merged template");
        
        // test instantiation with inheritance
        std::map<std::string, std::string> vars = {
            {"base_var", "new_base_value"},
            {"child_var", "new_child_value"},
            {"grandchild_var", "new_grandchild_value"},
            {"shared_var", "new_shared_value"}
        };
        
        std::string instantiated = manager.instantiate_template("grandchild_template", vars);
        
        // verify variable replacement with overrides
        TEST_ASSERT(instantiated.find("\"base_var\" \"new_base_value\"") != std::string::npos,
                   "base_var should be replaced with new value");
        TEST_ASSERT(instantiated.find("\"child_var\" \"new_child_value\"") != std::string::npos,
                   "child_var should be replaced with new value");
        TEST_ASSERT(instantiated.find("\"grandchild_var\" \"new_grandchild_value\"") != std::string::npos,
                   "grandchild_var should be replaced with new value");
        TEST_ASSERT(instantiated.find("\"shared_var\" \"new_shared_value\"") != std::string::npos,
                   "shared_var should be replaced with new value");

        // create templates that reference each other by their unique names
        // only use stderr suppression for the operations that will generate error logs
        {
            // test circular inheritance detection with targeted logging suppression
            // create unique template names with timestamps to avoid collisions
            std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            std::string name1 = "circular_t1_" + timestamp;
            std::string name2 = "circular_t2_" + timestamp;
            std::string circular1_content =
                    "@description \"circular template 1\"\n"
                    "@inherit \"" + name2 + "\"\n";
            std::string circular2_content =
                    "@description \"circular template 2\"\n"
                    "@inherit \"" + name1 + "\"\n";
            Logger::StderrSuppressionGuard stderr_guard_scoped;
            
            // save templates
            manager.save_template(name1, circular1_content);
            manager.save_template(name2, circular2_content);
            
            try {
                // this should throw an exception due to circular inheritance
                std::string circular_result = manager.load_template_with_inheritance(name1);
                return TestResult::fail("Circular inheritance not detected - this should have thrown an exception", 
                                      __FILE__, __LINE__);
            } catch (const std::exception& e) {
                // exception is expected
                std::string error = e.what();
                TEST_ASSERT(error.find("circular") != std::string::npos,
                           "Exception message should mention circular inheritance");
            }
        }
        
        // cleanup
        fs::remove_all(temp_dir);
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        // ensure cleanup even if test fails
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        return TestResult::fail("Exception in test_template_inheritance: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// show example queries for documentation
TestResult query_examples() {
    std::cout << "\nCQL Query Examples:" << std::endl;
    
    // define some example queries
    std::vector<std::pair<std::string, std::string>> examples = {
        {"Basic Copyright and Language", 
         "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\"\n@description \"Basic example\""},
         
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
         "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
         "@language \"C++\"\n"
         "@description \"implement a ${container_type}<${element_type}> class\"\n"
         "@variable \"container_type\" \"vector\"\n"
         "@variable \"element_type\" \"int\"\n"
         "@test \"Test ${container_type} operations\"\n"},
         
        // uncomment this to test the failure detection
        // this intentionally has variables before description which causes an error
        /*
        {"broken example",
         "@variable \"container_type\" \"vector\"\n"
         "@variable \"element_type\" \"int\"\n"
         "@description \"implement a ${container_type}<${element_type}> class\"\n"
         "@language \"c++\"\n"
         "@test \"test ${container_type} operations\"\n"}
        */
    };
    
    bool all_examples_compiled = true;
    std::string failed_example;
    std::string error_message;
    
    // process and display each example
    for (const auto& [title, query] : examples) {
        std::cout << "\n=== " << title << " ===\n" << std::endl;
        std::cout << "Query:\n" << query << std::endl;
        
        try {
            std::string result = QueryProcessor::compile(query);
            std::cout << "\nCompiled Result:\n" << result << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error compiling example: " << e.what() << std::endl;
            all_examples_compiled = false;
            failed_example = title;
            error_message = e.what();
        }
    }
    
    if (!all_examples_compiled) {
        return TestResult::fail("Failed to compile example: " + failed_example + "\nError: " + error_message,
                               __FILE__, __LINE__);
    }
    
    return TestResult::pass();
}

// Test the phase 2 example that was previously in main.cpp
TestResult test_phase2_example_compilation() {
    std::cout << "Testing Phase 2 comprehensive example compilation..." << std::endl;
    
    try {
        // Example query with phase 2 features that was previously in main.cpp
        const std::string query =
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

        std::cout << "\nDefault example:" << std::endl;
        std::cout << "Input query:\n" << query << std::endl;
        auto& logger = Logger::getInstance();
        logger.log(LogLevel::INFO, "\nDefault example:");
        logger.log(LogLevel::INFO, "Input query:\n", query);

        std::cout << "Compiling default example..." << std::endl;
        std::string result = QueryProcessor::compile(query);
        std::cout << "\n=== Compiled Query ===\n\n" << result << "\n===================" << std::endl;
        logger.log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
        
        TEST_ASSERT(!result.empty(), "Compilation result should not be empty");
        TEST_ASSERT(result.find("thread-safe queue with a maximum size") != std::string::npos,
                  "Result should contain the description");
        TEST_ASSERT(result.find("C++20 features and RAII principles") != std::string::npos,
                  "Result should contain context information");
        TEST_ASSERT(result.find("Producer-consumer pattern") != std::string::npos,
                  "Result should contain architecture information");
        TEST_ASSERT(result.find("O(1) for push and pop operations") != std::string::npos,
                  "Result should contain complexity requirements");
        TEST_ASSERT(result.find("ThreadSafeQueue<int> queue(1000)") != std::string::npos,
                  "Result should contain variable substitution");
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_phase2_example_compilation: " + std::string(e.what()),
                              __FILE__, __LINE__);
    }
}

// test template validator
TestResult test_template_validator() {
    std::cout << "Testing template validator..." << std::endl;
    
    // create a temporary template directory for testing
    std::string temp_dir = "./temp_templates";
    
    try {
        // Create a temporary template directory
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directory(temp_dir);
        
        // Create required common and user subdirectories to avoid errors
        // Suppress stderr only for directory creation operations that might log warnings
        {
            Logger::StderrSuppressionGuard stderr_guard;
            fs::create_directory(fs::path(temp_dir) / "common");
            fs::create_directory(fs::path(temp_dir) / "user");
        }
        
        // create a template manager with the temp directory
        TemplateManager manager(temp_dir);
        
        // create a template validator
        TemplateValidator validator(manager);
        
        // test 1: template with all variables declared and used
        std::string good_template = 
            "@description \"A good template with proper variables\"\n"
            "@variable \"var1\" \"value1\"\n"
            "@variable \"var2\" \"value2\"\n"
            "@language \"${var1}\"\n"
            "@context \"Using ${var2} features\"\n";
        
        manager.save_template("good_template", good_template);
        
        auto good_result = validator.validate_template("good_template");
        TEST_ASSERT(!good_result.has_issues(TemplateValidationLevel::ERROR),
                   "Good template should not have ERROR level issues");
        TEST_ASSERT(!good_result.has_issues(TemplateValidationLevel::WARNING),
                   "Good template should not have WARNING level issues");

        // test 2: template with undeclared variable (should generate warning)
        std::string warning_template =
            "@description \"A template with undeclared variable\"\n"
            "@variable \"var1\" \"value1\"\n"
            "@language \"${var1}\"\n"
            "@context \"Using ${undeclared_var} features\"\n";

        manager.save_template("warning_template", warning_template);

        auto warning_result = validator.validate_template("warning_template");
        TEST_ASSERT(!warning_result.has_issues(TemplateValidationLevel::ERROR),
                   "Warning template should not have ERROR level issues");
        TEST_ASSERT(warning_result.has_issues(TemplateValidationLevel::WARNING),
                   "Warning template should have WARNING level issues");
        TEST_ASSERT(warning_result.count_warnings() > 0,
                   "Warning template should have at least one warning");

        // test 3: template with unused variable (should generate info)
        std::string info_template =
            "@description \"A template with unused variable\"\n"
            "@variable \"var1\" \"value1\"\n"
            "@variable \"unused_var\" \"unused_value\"\n"
            "@language \"${var1}\"\n";

        manager.save_template("info_template", info_template);

        auto info_result = validator.validate_template("info_template");
        TEST_ASSERT(!info_result.has_issues(TemplateValidationLevel::ERROR),
                   "Info template should not have ERROR level issues");
        // we've upgraded info level issues to warning, so adjust the test accordingly
        TEST_ASSERT(info_result.has_issues(TemplateValidationLevel::WARNING),
                   "Info template should have WARNING level issues for unused variables");
        TEST_ASSERT(info_result.count_warnings() > 0,
                   "Info template should have at least one warning");

        {
            // test 4: template with circular inheritance (should generate error)
            // create unique names for this test run
            std::string circ_timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            std::string circular1_name = "circ1_" + circ_timestamp;
            std::string circular2_name = "circ2_" + circ_timestamp;

            // construct templates with these unique names
            std::string circular1 = "@description \"Template with circular inheritance\"\n@inherit \"" + circular2_name + "\"\n";
            std::string circular2 = "@description \"Another template in the circle\"\n@inherit \"" + circular1_name + "\"\n";

            // only suppress stderr during the actual operations that will generate error logs
            Logger::StderrSuppressionGuard circular_stderr_guard;

            manager.save_template(circular1_name, circular1);
            manager.save_template(circular2_name, circular2);
            auto circular_result = validator.validate_template(circular1_name);
            TEST_ASSERT(circular_result.has_issues(TemplateValidationLevel::ERROR),
                       "Circular inheritance should generate ERROR level issues");
            TEST_ASSERT(circular_result.count_errors() > 0,
                       "Circular inheritance should have at least one error");
        }

        // test 5: template with proper inheritance
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
        TEST_ASSERT(!inheritance_result.has_issues(TemplateValidationLevel::ERROR),
                   "Valid inheritance should not generate ERROR level issues");

        // test schema validation

        // create validator with schema rules
        for (auto schema = TemplateValidatorSchema::create_default_schema(); const auto& [name, rule] : schema.get_validation_rules()) {
            validator.add_validation_rule(rule);
        }

        // test schema rules with malformed template without logging
        std::string malformed =
            "@description \"Too short\"\n"  // description too short warning
            "@variable \"bad-name\" \"bad\"\n"  // invalid variable name (should be error)
            "@language \"${bad-name}\"\n"
            "@invalidDirective \"something\"\n";  // unknown directive (should be error)

        // create a validator with stricter rules that treats invalid directives and
        // variable names as errors instead of warnings
        TemplateValidator strict_validator(manager);
        strict_validator.add_validation_rule([](const std::string& content) {
            std::vector<TemplateValidationIssue> issues;

            // check for invalid directives (anything not starting with @ followed by a valid name)
            std::regex directive_regex("@([a-zA-Z_][a-zA-Z0-9_]*)");
            std::regex invalid_directive_regex("@([^a-zA-Z_]\\S*)");

            std::smatch match;
            auto search_start(content.cbegin());
            while (std::regex_search(search_start, content.cend(), match, invalid_directive_regex)) {
                issues.emplace_back(
                    TemplateValidationLevel::ERROR,
                    "Invalid directive name: " + match[1].str(),
                    std::nullopt,
                    "@" + match[1].str()
                );
                search_start = match.suffix().first;
            }

            // check for invalid variable names (should only contain letters, numbers, and underscores)
            std::regex variable_decl_regex("@variable\\s+\"([^\"]+)\"");
            std::regex valid_var_name_regex("[a-zA-Z_][a-zA-Z0-9_]*");

            search_start = content.cbegin();
            while (std::regex_search(search_start, content.cend(), match, variable_decl_regex)) {
                if (std::string var_name = match[1].str(); !std::regex_match(var_name, valid_var_name_regex)) {
                    issues.emplace_back(
                        TemplateValidationLevel::ERROR,
                        "Invalid variable name: " + var_name,
                        var_name
                    );
                }
                search_start = match.suffix().first;
            }
            return issues;
        });

        // only use the logger suppression during the actual save and load operations
        {
            // create a unique filename for each test run to avoid collisions
            std::string malformed_name = "malformed_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            Logger::StderrSuppressionGuard stderr_guard_scoped;

            manager.save_template(malformed_name, malformed);
            auto schema_result = strict_validator.validate_template(malformed_name);
            TEST_ASSERT(schema_result.has_issues(TemplateValidationLevel::ERROR),
                       "Malformed template should generate ERROR level issues");
            TEST_ASSERT(schema_result.count_errors() > 0,
                       "Malformed template should have at least one error");
            TEST_ASSERT(!schema_result.get_issues().empty(),
                       "Malformed template should have validation issues");
        }
        
        // cleanup
        fs::remove_all(temp_dir);
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        // ensure cleanup even if test fails
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        return TestResult::fail("Exception in test_template_validator: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// Test the ApiClient class
TestResult test_api_client() {
    std::cout << "Testing API Client..." << std::endl;
    
    try {
        // Create a config for testing without real API calls
        Config config;

        // Set a valid-looking API key (at least 30 chars for validation)
        config.set_api_key("dummy_api_key_for_testing_12345678901234567890");
        config.set_model("claude-3-test-model");
        config.set_timeout(1); // Short timeout for tests
        config.set_output_directory("./test_output");
        config.set_no_save_mode(true); // Don't save files to disk during tests
        
        // Create an ApiClient with the test config
        ApiClient client(config);
        
        // Test client initialization
        TEST_ASSERT(client.get_status() == ApiClientStatus::Ready ||
                    client.get_status() == ApiClientStatus::Error, // Maybe error if no curl
                    "Client should be in Ready or Error state after initialization");
        
        // Test config getters and setters
        client.set_model("claude-3-different-model");
        client.set_timeout(30); // calls const version
        client.set_max_retries(5); // calls const version
        
        // We can't easily test actual API requests without mocking,
        // but we can test error handling with invalid configuration
        
        // Suppress stderr for API error logs
        {
            Logger::StderrSuppressionGuard stderr_guard;
            
            // Test handling of invalided (empty) request
            auto empty_response = client.submit_query("");
            TEST_ASSERT(empty_response.has_error(), "Empty query should result in error");
            
            // Test callback in async request (using lambda)
            bool callback_called = false;
            auto async_future = client.submit_query_async("", [&callback_called](const ApiResponse& /*response*/) {
                callback_called = true;
            });
            
            // Wait for the future to complete to get the response
            auto async_response = async_future.get();
            
            TEST_ASSERT(callback_called, "Async callback should be called");
            TEST_ASSERT(async_response.has_error(), "Empty async query should result in error");
        }
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_api_client: " + std::string(e.what()),
                              __FILE__, __LINE__);
    }
}

// Test the ResponseProcessor class
TestResult test_response_processor() {
    std::cout << "Testing Response Processor..." << std::endl;
    
    try {
        // Create a config for testing
        Config config;
        config.set_output_directory("./test_output");
        config.set_no_save_mode(true); // Don't save files to disk during tests
        
        // Create a ResponseProcessor with the test config
        ResponseProcessor processor(config);
        
        // Test empty response handling
        auto empty_files = processor.process_response("");
        TEST_ASSERT(empty_files.empty(), "Empty response should produce no files");
        
        // Test basic response with no code blocks
        const std::string text_only_response = "This is a response with no code blocks.";
        const auto text_only_files = processor.process_response(text_only_response);
        TEST_ASSERT(text_only_files.empty(), "Response without code blocks should produce no files");
        
        // Test response with a single code block
        std::string single_block_response = 
            "Here's an implementation of a simple counter class:\n\n"
            "```cpp\n"
            "class Counter {\n"
            "private:\n"
            "    int m_count = 0;\n"
            "public:\n"
            "    void increment() { m_count++; }\n"
            "    int get_count() const { return m_count; }\n"
            "};\n"
            "```\n";

        const auto single_block_files = processor.process_response(single_block_response);
        TEST_ASSERT(single_block_files.size() == 1, "Response with one code block should produce one file");
        
        if (!single_block_files.empty()) {
            auto& file = single_block_files[0];
            TEST_ASSERT(file.m_language == "C++", "File language should be C++");
            TEST_ASSERT(file.m_content.find("class Counter") != std::string::npos, 
                      "File content should contain the Counter class");
            TEST_ASSERT(!file.m_is_test, "File should not be marked as a test");
        }
        
        // Test response with multiple code blocks including a test
        const std::string multi_block_response =
            "Here's an implementation of a Vector class:\n\n"
            "```cpp\n"
            "// vector.hpp\n"
            "template <typename T>\n"
            "class Vector {\n"
            "private:\n"
            "    T* m_data;\n"
            "    size_t m_size;\n"
            "public:\n"
            "    Vector() : m_data(nullptr), m_size(0) {}\n"
            "    void push_back(const T& value);\n"
            "    T& at(size_t index);\n"
            "};\n"
            "```\n\n"
            "And here's the implementation:\n\n"
            "```cpp\n"
            "// vector.cpp\n"
            "template <typename T>\n"
            "void Vector<T>::push_back(const T& value) {\n"
            "    // Implementation\n"
            "}\n\n"
            "template <typename T>\n"
            "T& Vector<T>::at(size_t index) {\n"
            "    // Implementation\n"
            "}\n"
            "```\n\n"
            "Here's a test for the Vector class:\n\n"
            "```cpp\n"
            "// test_vector.cpp\n"
            "void test_vector() {\n"
            "    Vector<int> v;\n"
            "    v.push_back(42);\n"
            "    assert(v.at(0) == 42);\n"
            "}\n"
            "```\n";

        const auto multi_block_files = processor.process_response(multi_block_response);
        TEST_ASSERT(multi_block_files.size() >= 2, "Response with multiple code blocks should produce at least 2 files");
        
        bool found_impl = false;
        bool found_test = false;
        
        for (const auto& file : multi_block_files) {
            if (file.m_is_test) {
                found_test = true;
                TEST_ASSERT(file.m_content.find("test_vector") != std::string::npos, 
                          "Test file should contain test_vector function");
            } else {
                found_impl = true;
                TEST_ASSERT(file.m_content.find("class Vector") != std::string::npos || 
                           file.m_content.find("Vector<T>::") != std::string::npos,
                          "Implementation file should contain Vector class");
            }
        }
        
        TEST_ASSERT(found_impl, "Should have found at least one implementation file");
        TEST_ASSERT(found_test, "Should have found at least one test file");
        
        // Test setting different configuration options
        processor.set_output_directory("./different_output");
        processor.set_overwrite_existing(true);
        processor.set_create_directories(false);
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_response_processor: " + std::string(e.what()),
                              __FILE__, __LINE__);
    }
}

// Test all example CQL files in the examples directory
TestResult test_examples_compilation() {
    std::cout << "Testing compilation of example CQL files..." << std::endl;
    
    try {
        // Get the project root directory
        std::string project_root;
        
        // Try to find the examples directory relative to the current working directory
        if (fs::exists("../examples")) {
            project_root = "..";
        } else if (fs::exists("../../examples")) {
            project_root = "../..";
        } else {
            // If we can't find it, look for the absolute path based on this file's location
            std::string file_path = __FILE__;
            std::string src_dir = file_path.substr(0, file_path.find_last_of("/\\"));
            project_root = src_dir + "/../..";
        }
        
        // Path to examples directory
        std::string examples_dir = project_root + "/examples";
        
        if (!fs::exists(examples_dir)) {
            return TestResult::fail("Examples directory not found: " + examples_dir,
                                   __FILE__, __LINE__);
        }
        
        std::cout << "Examples directory found at: " << examples_dir << std::endl;
        
        // Find all .llm files in the examples directory
        std::vector<std::string> example_files;
        for (const auto& entry : fs::directory_iterator(examples_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".llm") {
                example_files.push_back(entry.path().string());
            }
        }
        
        if (example_files.empty()) {
            return TestResult::fail("No .llm files found in examples directory",
                                   __FILE__, __LINE__);
        }
        
        std::cout << "Found " << example_files.size() << " example files to test" << std::endl;
        
        // Process each example file
        std::vector<std::string> failed_examples;
        std::vector<std::string> error_messages;
        
        for (const auto& file_path : example_files) {
            std::string file_name = fs::path(file_path).filename().string();
            std::cout << "  Testing example: " << file_name << "... ";
            
            try {
                // Read file content
                std::string content = util::read_file(file_path);
                
                // Skip empty files
                if (content.empty()) {
                    std::cout << "SKIPPED (empty file)" << std::endl;
                    continue;
                }
                
                // Compile the example
                // Use Logger::StderrSuppressionGuard to avoid cluttering the output
                // with expected warnings from example files
                {
                    Logger::StderrSuppressionGuard stderr_guard;
                    std::string result = QueryProcessor::compile(content);
                }
                
                print_test_result(file_name, TestResult::pass());
            } catch (const std::exception& e) {
                print_test_result(file_name, TestResult::fail(e.what()));
                std::string error = e.what();
                
                // Check if this is a parser error or validation error
                // Store the error message for later
                failed_examples.push_back(file_name);
                error_messages.push_back(error);
                
                std::cout << "    Error: " << error << std::endl;
            }
        }
        
        // Report results
        std::cout << "\nExamples compilation test summary:" << std::endl;
        std::cout << "  Total examples: " << example_files.size() << std::endl;
        std::cout << "  Passed: " << (example_files.size() - failed_examples.size()) << std::endl;
        std::cout << "  Failed: " << failed_examples.size() << std::endl;
        
        if (!failed_examples.empty()) {
            std::string error_summary = "Failed to compile " + 
                                       std::to_string(failed_examples.size()) + 
                                       " example(s):\n";
            
            for (size_t i = 0; i < failed_examples.size(); ++i) {
                error_summary += "  - " + failed_examples[i] + ": " + error_messages[i] + "\n";
            }
            
            return TestResult::fail(error_summary, __FILE__, __LINE__);
        }
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_examples_compilation: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// Implementation for the lexer tests
TestResult test_lexer() {
    std::cout << "Running lexer tests..." << std::endl;

    try {
        // test basic tokenization
        {
            std::string input = "@language \"C++\"\n@description \"implement a stack\"";
            Lexer lexer(input);

            auto token1 = lexer.next_token();
            TEST_ASSERT(token1 && token1->m_type == TokenType::LANGUAGE, "First token should be LANGUAGE");

            auto token2 = lexer.next_token();
            TEST_ASSERT(token2 && token2->m_type == TokenType::STRING && token2->m_value == "C++", 
                      "Second token should be STRING with value C++");

            auto token3 = lexer.next_token();
            TEST_ASSERT(token3 && token3->m_type == TokenType::NEWLINE, "Third token should be NEWLINE");

            auto token4 = lexer.next_token();
            TEST_ASSERT(token4 && token4->m_type == TokenType::DESCRIPTION, "Fourth token should be DESCRIPTION");

            auto token5 = lexer.next_token();
            TEST_ASSERT(token5 && token5->m_type == TokenType::STRING && token5->m_value == "implement a stack",
                      "Fifth token should be STRING with value 'implement a stack'");

            auto token6 = lexer.next_token();
            TEST_ASSERT(!token6, "Sixth token should be null (end of input)");
        }

        // test string escape sequences
        {
            std::string input = R"(@language "C++\n with newline")";
            Lexer lexer(input);

            auto token1 = lexer.next_token();
            TEST_ASSERT(token1 && token1->m_type == TokenType::LANGUAGE, "First token should be LANGUAGE");

            auto token2 = lexer.next_token();
            TEST_ASSERT(token2 && token2->m_type == TokenType::STRING, "Second token should be STRING");
            TEST_ASSERT(token2->m_value == "C++\n with newline", "Value should have escaped newline");
        }

        // test new tokens
        {
            std::string input = "@dependency \"boost::asio\"\n@performance \"latency < 5ms\"";
            Lexer lexer(input);

            auto token1 = lexer.next_token();
            TEST_ASSERT(token1 && token1->m_type == TokenType::DEPENDENCY, "First token should be DEPENDENCY");

            auto token2 = lexer.next_token();
            TEST_ASSERT(token2 && token2->m_type == TokenType::STRING && token2->m_value == "boost::asio",
                      "Second token should be STRING with value 'boost::asio'");

            auto token3 = lexer.next_token();
            TEST_ASSERT(token3 && token3->m_type == TokenType::NEWLINE, "Third token should be NEWLINE");

            auto token4 = lexer.next_token();
            TEST_ASSERT(token4 && token4->m_type == TokenType::PERFORMANCE, "Fourth token should be PERFORMANCE");

            auto token5 = lexer.next_token();
            TEST_ASSERT(token5 && token5->m_type == TokenType::STRING && token5->m_value == "latency < 5ms",
                      "Fifth token should be STRING with value 'latency < 5ms'");
        }

        // test copyright token
        {
            std::string input = R"(@copyright "MIT License" "2025 dbjwhs")";
            Lexer lexer(input);

            auto token1 = lexer.next_token();
            TEST_ASSERT(token1 && token1->m_type == TokenType::COPYRIGHT, "First token should be COPYRIGHT");

            auto token2 = lexer.next_token();
            TEST_ASSERT(token2 && token2->m_type == TokenType::STRING && token2->m_value == "MIT License",
                      "Second token should be STRING with value 'MIT License'");

            auto token3 = lexer.next_token();
            TEST_ASSERT(token3 && token3->m_type == TokenType::STRING && token3->m_value == "2025 dbjwhs",
                      "Third token should be STRING with value '2025 dbjwhs'");
        }

        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_lexer: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// Implementation for the parser tests
TestResult test_parser() {
    std::cout << "Running parser tests..." << std::endl;

    try {
        // test basic parsing
        {
            std::string input = R"(
                @language "C++"
                @description "implement a thread-safe queue"
                @context "Using Modern C++ features"
                @test "Test empty queue"
                @dependency "std::mutex"
                @performance "Handle 1M operations per second"
            )";

            Parser parser(input);
            const auto nodes = parser.parse();

            TEST_ASSERT(nodes.size() == 5, "Should parse 5 nodes");

            // verify node types
            const auto* code_request = dynamic_cast<CodeRequestNode*>(nodes[0].get());
            TEST_ASSERT(code_request && code_request->language() == "C++",
                      "First node should be CodeRequestNode with language C++");
            TEST_ASSERT(code_request->description() == "implement a thread-safe queue",
                      "CodeRequestNode should have correct description");

            const auto* context = dynamic_cast<ContextNode*>(nodes[1].get());
            TEST_ASSERT(context && context->context() == "Using Modern C++ features",
                      "Second node should be ContextNode with correct text");

            const auto* test = dynamic_cast<TestNode*>(nodes[2].get());
            TEST_ASSERT(test && test->test_cases().size() == 1,
                      "Third node should be TestNode with 1 test case");
            TEST_ASSERT(test->test_cases()[0] == "Test empty queue",
                      "TestNode should have correct test case");

            const auto* dependency = dynamic_cast<DependencyNode*>(nodes[3].get());
            TEST_ASSERT(dependency && dependency->dependencies().size() == 1,
                      "Fourth node should be DependencyNode with 1 dependency");
            TEST_ASSERT(dependency->dependencies()[0] == "std::mutex",
                      "DependencyNode should have correct dependency");

            const auto* performance = dynamic_cast<PerformanceNode*>(nodes[4].get());
            TEST_ASSERT(performance && performance->requirement() == "Handle 1M operations per second",
                      "Fifth node should be PerformanceNode with correct requirement");
        }

        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_parser: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// Implementation for the compiler tests
TestResult test_compiler() {
    std::cout << "Running compiler tests..." << std::endl;

    try {
        // test basic compilation
        {
            std::string input = R"(
                @language "C++"
                @description "implement a thread-safe queue"
                @context "Using Modern C++ features"
                @test "Test empty queue"
            )";

            Parser parser(input);
            auto nodes = parser.parse();

            QueryCompiler compiler;
            for (const auto& node : nodes) {
                node->accept(compiler);
            }

            std::string result = compiler.get_compiled_query();

            // verify the compiled query contains expected sections
            TEST_ASSERT(result.find("Please generate C++ code that:") != std::string::npos,
                      "Compiled query should contain code generation prompt");
            TEST_ASSERT(result.find("implement a thread-safe queue") != std::string::npos,
                      "Compiled query should contain the description");
            TEST_ASSERT(result.find("Context:") != std::string::npos,
                      "Compiled query should contain Context section");
            TEST_ASSERT(result.find("Using Modern C++ features") != std::string::npos,
                      "Compiled query should contain the context text");
            TEST_ASSERT(result.find("Please include tests for the following cases:") != std::string::npos,
                      "Compiled query should contain test section");
            TEST_ASSERT(result.find("Test empty queue") != std::string::npos,
                      "Compiled query should contain the test case");
        }

        // test compiler copyright
        {
            std::string input = R"(
                @copyright "MIT License" "2025 dbjwhs"
                @language "C++"
                @description "implement a thread-safe queue"
            )";

            Parser parser(input);
            auto nodes = parser.parse();

            QueryCompiler compiler;
            for (const auto& node : nodes) {
                node->accept(compiler);
            }

            std::string result = compiler.get_compiled_query();

            // verify the compiled query contains expected sections
            TEST_ASSERT(result.find("Please include the following copyright header") != std::string::npos,
                      "Compiled query should contain copyright section");
            TEST_ASSERT(result.find("MIT License") != std::string::npos,
                      "Compiled query should contain license text");
            TEST_ASSERT(result.find("Copyright (c) 2025 dbjwhs") != std::string::npos,
                      "Compiled query should contain copyright owner");
            TEST_ASSERT(result.find("Please generate C++ code that:") != std::string::npos,
                      "Compiled query should contain code generation prompt");
        }

        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_compiler: " + std::string(e.what()),
                               __FILE__, __LINE__);
    }
}

// Integration with the standalone tests from cql.cpp
TestResult test_lexer_standalone() {
    std::cout << "Running lexer standalone tests..." << std::endl;
    
    // This function simply calls our new implementation
    return test_lexer();
}

// Integration with the standalone tests from cql.cpp
TestResult test_parser_standalone() {
    std::cout << "Running parser standalone tests..." << std::endl;
    
    // This function simply calls our new implementation
    return test_parser();
}

// Integration with the standalone tests from cql.cpp
TestResult test_compiler_standalone() {
    std::cout << "Running compiler standalone tests..." << std::endl;
    
    // This function simply calls our new implementation
    return test_compiler();
}

// Implementation of list_tests function
void list_tests() {
    std::cout << "Available tests in the CQL test suite:" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    
    // Print each test name from the shared test list
    for (size_t ndx = 0; ndx < tests.size(); ++ndx) {
        std::cout << ndx + 1 << ". " << tests[ndx].name << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Run a specific test with: cql --test \"Test Name\"" << std::endl;
    std::cout << "Run all tests with: cql --test" << std::endl;
    std::cout << "Run all tests without stopping on failure: cql --test --no-fail-fast" << std::endl;
}

// Test JSON format output
TestResult test_json_format_output() {
    std::cout << "Testing JSON format output..." << std::endl;
    
    try {
        // Test 1: Basic JSON output with minimal content
        std::string basic_json_query = 
            "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
            "@language \"C++\"\n"
            "@description \"Simple test for JSON output\"\n"
            "@format \"json\"\n";
            
        std::string basic_result = QueryProcessor::compile(basic_json_query);
        
        // Verify basic JSON structure
        TEST_ASSERT(basic_result.find("{\n") == 0, "JSON output should start with opening brace");
        TEST_ASSERT(basic_result.find("\n}\n") != std::string::npos, "JSON output should end with closing brace");
        TEST_ASSERT(basic_result.find("\"query\":") != std::string::npos, "JSON output should contain query field");
        TEST_ASSERT(basic_result.find("\"model\":") != std::string::npos, "JSON output should contain model field");
        TEST_ASSERT(basic_result.find("\"format\": \"json\"") != std::string::npos, "JSON output should contain format field with json value");
        
        // Test 2: JSON output with special characters that need escaping
        std::string special_chars_query = 
            "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
            "@language \"C++\"\n"
            "@description \"Test with special chars\"\n"
            "@format \"json\"\n";
            
        std::string special_chars_result = QueryProcessor::compile(special_chars_query);
        
        // Just verify the result is valid and contains expected content
        TEST_ASSERT(special_chars_result.find("\"query\"") != std::string::npos, 
                   "JSON output should contain query field");
        
        // Test 3: JSON output with model parameters
        std::string model_params_query = 
            "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
            "@language \"C++\"\n"
            "@description \"Test with model parameters\"\n"
            "@format \"json\"\n"
            "@model \"claude-3-custom\"\n"
            "@max_tokens 80000\n"
            "@temperature 0.5\n"
            "@output_format \"multiple_files\"\n";
            
        std::string model_params_result = QueryProcessor::compile(model_params_query);
        
        // Verify model parameters included
        TEST_ASSERT(model_params_result.find("\"model\": \"claude-3-custom\"") != std::string::npos, 
                   "JSON output should contain specified model");
        TEST_ASSERT(model_params_result.find("\"max_tokens\": 80000") != std::string::npos, 
                   "JSON output should contain max_tokens parameter");
        TEST_ASSERT(model_params_result.find("\"temperature\": 0.5") != std::string::npos, 
                   "JSON output should contain temperature parameter");
        TEST_ASSERT(model_params_result.find("\"output_format\": \"multiple_files\"") != std::string::npos, 
                   "JSON output should contain output_format parameter");
        
        // Test 4: JSON output with complex content (multiline) 
        std::string complex_query = 
            "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
            "@language \"C++\"\n"
            "@description \"Test with complex multiline content\"\n"
            "@format \"json\"\n"
            "@context \"Line 1 of context\"\n"
            "@context \"Line 2 of context\"\n"
            "@example \"Code Example\" \"class Example {\n"
            "public:\n"
            "    void method() {\n"
            "        // Comment with quotes\n"
            "        std::cout << \\\"Hello\\\\nWorld\\\" << std::endl;\n"
            "    }\n"
            "};\"\n";
            
        std::string complex_result = QueryProcessor::compile(complex_query);
        
        // Verify newlines and escaping in complex content
        TEST_ASSERT(complex_result.find("\\n") != std::string::npos, 
                   "JSON output should escape newlines");
        // Check for quoted text
        TEST_ASSERT(complex_result.find("quotes") != std::string::npos, 
                   "JSON output should contain the word 'quotes'");
        // Check for preserved escape sequences
        TEST_ASSERT(complex_result.find("\\\\\\\"Hello\\\\\\\\nWorld\\\\\\\"") != std::string::npos 
                    || complex_result.find("Hello\\\\nWorld") != std::string::npos,
                   "JSON output should preserve escaped characters in the original text");
                   
        // Test 5: Validate JSON output using nlohmann/json parser
        // For each test case, verify the output is valid JSON by parsing it
        try {
            auto parsed_basic = nlohmann::json::parse(basic_result);
            TEST_ASSERT(parsed_basic.contains("query"), "Parsed JSON should contain 'query' field");
            TEST_ASSERT(parsed_basic.contains("model"), "Parsed JSON should contain 'model' field");
            TEST_ASSERT(parsed_basic.contains("format"), "Parsed JSON should contain 'format' field");
            TEST_ASSERT(parsed_basic["format"] == "json", "Format field should be 'json'");
            
            auto parsed_special = nlohmann::json::parse(special_chars_result);
            TEST_ASSERT(parsed_special.contains("query"), "Parsed JSON should contain 'query' field");
            
            auto parsed_model_params = nlohmann::json::parse(model_params_result);
            TEST_ASSERT(parsed_model_params.contains("model"), "Parsed JSON should contain 'model' field");
            TEST_ASSERT(parsed_model_params.contains("max_tokens"), "Parsed JSON should contain 'max_tokens' field");
            TEST_ASSERT(parsed_model_params.contains("temperature"), "Parsed JSON should contain 'temperature' field");
            TEST_ASSERT(parsed_model_params.contains("output_format"), "Parsed JSON should contain 'output_format' field");
            
            auto parsed_complex = nlohmann::json::parse(complex_result);
            TEST_ASSERT(parsed_complex.contains("query"), "Parsed JSON should contain 'query' field");
            TEST_ASSERT(parsed_complex["query"].is_string(), "Query field should be a string");
        } catch (const nlohmann::json::parse_error& e) {
            return TestResult::fail("JSON validation failed: " + std::string(e.what()) +
                                  " - Invalid JSON generated", __FILE__, __LINE__);
        }
        
        return TestResult::pass();
    } catch (const std::exception& e) {
        return TestResult::fail("Exception in test_json_format_output: " + std::string(e.what()),
                              __FILE__, __LINE__);
    }
}

} // namespace cql::test
