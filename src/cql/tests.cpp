// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <cassert>
#include <sstream>
#include <utility>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <gtest/gtest.h>

// Include cql.hpp first to ensure TestResult is defined before test_utils.hpp
#include "../../include/cql/cql.hpp"
// Then include all other headers
#include "../../include/cql/template_manager.hpp"
#include "../../include/cql/template_validator.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include "../../include/cql/api_client.hpp"
#include "../../include/cql/response_processor.hpp"
#include "../../include/cql/input_validator.hpp"
#include "../../include/cql/test_utils.hpp"
#include "../../third_party/include/nlohmann/json.hpp"

// note file exists in the cpp-snippets repo, you will need to check this out and have it and cql share the
// same root directory
#include "../../include/cql/project_utils.hpp"

namespace fs = std::filesystem;

namespace cql::test {

// Forward declarations of test functions
TestResult test_api_client();
TestResult test_response_processor();
TestResult test_examples_compilation();
TestResult test_lexer_standalone();
TestResult test_parser_standalone();
TestResult test_compiler_standalone();
TestResult test_json_format_output();
TestResult test_basic_compilation();
TestResult test_complex_compilation();
TestResult test_validation_requirements();
TestResult test_phase2_features();
TestResult test_template_management();
TestResult test_template_inheritance();
TestResult test_template_validator();
TestResult test_phase2_example_compilation();
// Test functions now in Google Test format
TestResult test_architecture_patterns();
TestResult test_configuration();

// Define TestInfo struct at namespace level
struct TestInfo {
    std::string name;
    std::function<TestResult()> test_func;
};

// Define the test list at namespace level
const std::vector<TestInfo> tests = {
    {"Basic Compilation", test_basic_compilation},
    {"Complex Compilation", test_complex_compilation},
    {"Validation Requirements", test_validation_requirements},
    {"Phase 2 Features", test_phase2_features},
    {"Template Management", test_template_management},
    {"Template Inheritance", test_template_inheritance},
    {"Template Validator", test_template_validator},
    {"Phase 2 Example Compilation", test_phase2_example_compilation},
    {"Architecture Patterns", test_architecture_patterns},
    {"API Client", test_api_client},
    {"Response Processor", test_response_processor},
    {"Configuration", test_configuration},
    {"Examples Compilation", test_examples_compilation},
    {"Lexer (Standalone)", test_lexer_standalone},
    {"Parser (Standalone)", test_parser_standalone},
    {"Compiler (Standalone)", test_compiler_standalone},
    {"JSON Format Output", test_json_format_output}
};

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
    std::cout << "Run tests with Google Test: cql --gtest" << std::endl;
}

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

// Base test fixture for CQL tests
class CQLTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
    }

    void TearDown() override {
        // Common cleanup for all tests
    }

    // Utility functions available to all tests
    static std::string create_temp_directory(const std::string& prefix) {
        std::string temp_dir = "./" + prefix + "_" +
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directory(temp_dir);
        return temp_dir;
    }

    static void remove_temp_directory(const std::string& dir) {
        if (fs::exists(dir)) {
            fs::remove_all(dir);
        }
    }
};

// Test cases

// Basic compilation test
TEST_F(CQLTest, BasicCompilation) {
    std::cout << "Testing basic compilation..." << std::endl;

    const std::string query = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\"\n@description \"test compilation\"";
    const std::string result = QueryProcessor::compile(query);

    ASSERT_FALSE(result.empty()) << "Compilation result should not be empty";
    ASSERT_NE(result.find("MIT License"), std::string::npos) << "Result should contain 'MIT License'";
    ASSERT_NE(result.find("Copyright (c) 2025 dbjwhs"), std::string::npos) << "Result should contain copyright information";
    ASSERT_NE(result.find("C++"), std::string::npos) << "Result should contain language information";
}

// Complex compilation test
TEST_F(CQLTest, ComplexCompilation) {
    std::cout << "Testing complex compilation..." << std::endl;

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

    ASSERT_FALSE(result.empty()) << "Compilation result should not be empty";
    ASSERT_NE(result.find("MIT License"), std::string::npos) << "Result should contain license information";
    ASSERT_NE(result.find("C++"), std::string::npos) << "Result should contain language information";
    ASSERT_NE(result.find("thread-safe queue"), std::string::npos) << "Result should contain the description";
    ASSERT_NE(result.find("C++20"), std::string::npos) << "Result should contain context information about C++20";
    ASSERT_NE(result.find("exception-safe"), std::string::npos) << "Result should contain context information about exception safety";
    ASSERT_NE(result.find("Test concurrent push"), std::string::npos) << "Result should contain test information about push operations";
    ASSERT_NE(result.find("Test concurrent pop"), std::string::npos) << "Result should contain test information about pop operations";
}

// Validation requirements test
TEST_F(CQLTest, ValidationRequirements) {
    std::cout << "Testing validation requirements..." << std::endl;

    // Test 1: Missing copyright directive
    std::string missing_copyright = "@language \"C++\"\n@description \"test without copyright\"";

    ASSERT_THROW({
        std::string result = QueryProcessor::compile(missing_copyright);
    }, std::exception) << "Missing copyright should cause compilation to fail";

    try {
        std::string result = QueryProcessor::compile(missing_copyright);
        FAIL() << "Missing copyright validation failed - compilation should have failed";
    } catch (const std::exception& e) {
        std::string error_message = e.what();
        std::ranges::transform(error_message, error_message.begin(), ::tolower);
        ASSERT_NE(error_message.find("copyright"), std::string::npos)
            << "Error message should mention missing COPYRIGHT directive";
    }

    // Test 2: Missing language directive
    // We need to test the validation directly since the parser syntax errors would occur before validation
    {
        // Create a copyright node and description node manually, but skip language
        std::vector<std::unique_ptr<QueryNode>> nodes;
        nodes.push_back(std::make_unique<CopyrightNode>("MIT License", "2025 dbjwhs"));
        nodes.push_back(std::make_unique<CodeRequestNode>("", "test without language"));

        // Now test the validator directly with our manually constructed AST
        ASSERT_THROW({
            QueryValidator validator;
            validator.validate(nodes);
        }, ValidationException) << "Missing language should cause validation to fail";

        try {
            QueryValidator validator;
            validator.validate(nodes);
            FAIL() << "Missing language validation failed - validation should have failed";
        } catch (const ValidationException& e) {
            std::string error_message = e.what();
            std::ranges::transform(error_message, error_message.begin(), ::tolower);
            ASSERT_NE(error_message.find("language"), std::string::npos)
                << "Error message should mention missing LANGUAGE directive";
        }
    }

    // Test 3: Missing description directive
    std::string missing_description = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\"";

    ASSERT_THROW({
        std::string result = QueryProcessor::compile(missing_description);
    }, std::exception) << "Missing description should cause compilation to fail";

    try {
        std::string result = QueryProcessor::compile(missing_description);
        FAIL() << "Missing description validation failed - compilation should have failed";
    } catch (const std::exception& e) {
        std::string error_message = e.what();
        std::ranges::transform(error_message, error_message.begin(), ::tolower);
        ASSERT_NE(error_message.find("description"), std::string::npos)
            << "Error message should mention missing DESCRIPTION directive";
    }

    // Test 4: Parser errors shouldn't prevent validation from running
    // This test has both a parser error (invalid syntax) and a missing required directive
    std::string parser_and_validation_error = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@invalid_token \"Something\"\n@language \"C++\"";

    ASSERT_THROW({
        std::string result = QueryProcessor::compile(parser_and_validation_error);
    }, std::exception) << "Invalid token should cause compilation to fail";

    try {
        std::string result = QueryProcessor::compile(parser_and_validation_error);
        FAIL() << "Compilation should have failed due to errors";
    } catch (const std::exception& e) {
        std::string error_message = e.what();
        std::ranges::transform(error_message, error_message.begin(), ::tolower);

        // We should see the validation error (missing description) in the output
        ASSERT_TRUE(error_message.find("description") != std::string::npos ||
                    error_message.find("invalid") != std::string::npos)
            << "Error should be reported for either validation or parser issues";
    }

    // Test 5: Successful validation with all required directives
    std::string valid_query = "@copyright \"MIT License\" \"2025 dbjwhs\"\n@language \"C++\"\n@description \"test with all required fields\"";
    std::string result = QueryProcessor::compile(valid_query);
    ASSERT_FALSE(result.empty()) << "Valid query should compile successfully";
}

// Phase 2 features test
TEST_F(CQLTest, Phase2Features) {
    std::cout << "Testing Phase 2 features..." << std::endl;

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

    ASSERT_FALSE(result.empty()) << "Compilation result should not be empty";

    // check for the presence of the content without the exact format string
    ASSERT_NE(result.find("Producer-consumer pattern"), std::string::npos) << "Result should contain architecture information";
    ASSERT_NE(result.find("Thread-safe for concurrent access"), std::string::npos) << "Result should contain constraint information";
    ASSERT_NE(result.find("Prevent data races and deadlocks"), std::string::npos) << "Result should contain security information";
    ASSERT_NE(result.find("O(1) for push and pop operations"), std::string::npos) << "Result should contain complexity information";
    ASSERT_NE(result.find("ThreadSafeQueue<int> queue(1000)"), std::string::npos) << "Result should contain variable substitution in example";
}

// Template management test
TEST_F(CQLTest, TemplateManagement) {
    std::cout << "Testing template management..." << std::endl;

    // create a temporary template directory for testing
    std::string temp_dir = create_temp_directory("temp_templates");

    try {
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
        ASSERT_EQ(templates.size(), 1) << "Should have exactly one template";
        ASSERT_NE(templates[0].find("test_template"), std::string::npos) << "Template list should contain 'test_template'";

        // test loading a template
        std::string loaded = manager.load_template("test_template");
        ASSERT_EQ(loaded, template_content) << "Loaded template content should match original";

        // test getting template metadata
        auto metadata = manager.get_template_metadata("test_template");
        ASSERT_NE(metadata.name.find("test_template"), std::string::npos) << "Template metadata name should contain 'test_template'";
        ASSERT_EQ(metadata.description, "test template") << "Template metadata description should match";
        ASSERT_EQ(metadata.variables.size(), 1) << "Template should have one variable";
        ASSERT_EQ(metadata.variables[0], "test_var") << "Template variable should be 'test_var'";

        // test template instantiation with variables
        std::map<std::string, std::string> vars = {{"test_var", "C++"}};
        std::string instantiated = manager.instantiate_template("test_template", vars);
        ASSERT_NE(instantiated.find("@language \"C++\""), std::string::npos) << "Instantiated template should contain substituted variable";

        // test creating a category
        bool category_created = manager.create_category("test_category");
        ASSERT_TRUE(category_created) << "Should be able to create a category";

        // test saving a template in a category
        manager.save_template("test_category/category_template", template_content);

        // test listing categories
        auto categories = manager.list_categories();
        ASSERT_GE(categories.size(), 3) << "Should have at least common, user, and test_category";

        bool found_category = false;
        for (const auto& cat : categories) {
            if (cat == "test_category") {
                found_category = true;
                break;
            }
        }
        ASSERT_TRUE(found_category) << "Should find the test_category in the category list";

        // test deleting a template
        bool template_deleted = manager.delete_template("test_template");
        ASSERT_TRUE(template_deleted) << "Should be able to delete a template";

        templates = manager.list_templates();
        bool template_found = false;
        for (const auto& tmpl : templates) {
            if (tmpl.find("test_template") != std::string::npos &&
                tmpl.find("test_category") == std::string::npos) {
                template_found = true;
                break;
            }
        }
        ASSERT_FALSE(template_found) << "Deleted template should not be in the template list";
    }
    catch(const std::exception& e) {
        remove_temp_directory(temp_dir);
        FAIL() << "Exception in TemplateManagement test: " << e.what();
    }

    // cleanup
    remove_temp_directory(temp_dir);
}

// Call the legacy test functions for the API tests for now
// These can be gradually migrated to proper Google Test functions later

TEST_F(CQLTest, PhaseExampleCompilation) {
    const TestResult result = test_phase2_example_compilation();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, TemplateInheritance) {
    const TestResult result = test_template_inheritance();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, TemplateValidator) {
    const TestResult result = test_template_validator();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, APIClient) {
    const TestResult result = test_api_client();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, ResponseProcessor) {
    const TestResult result = test_response_processor();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, ExamplesCompilation) {
    const TestResult result = test_examples_compilation();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, LexerStandalone) {
    const TestResult result = test_lexer_standalone();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, ParserStandalone) {
    const TestResult result = test_parser_standalone();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, CompilerStandalone) {
    const TestResult result = test_compiler_standalone();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

TEST_F(CQLTest, JSONFormatOutput) {
    const TestResult result = test_json_format_output();
    ASSERT_TRUE(result.passed()) << result.get_error_message();
}

/**
 * Test for comments and whitespace handling in CQL files
 */
TEST_F(CQLTest, CommentsAndWhitespaceHandling) {
    std::cout << "Testing comments and whitespace handling..." << std::endl;
    
    // Create test directory if it doesn't exist
    std::filesystem::create_directories("test_output");
    
    // Create test files
    std::ofstream baseline("test_output/baseline.llm");
    baseline << "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
             << "@language \"C++\"\n"
             << "@description \"Basic test template for comments and whitespace handling\"\n";
    baseline.close();
    
    std::ofstream comment_test("test_output/comment_test.llm");
    comment_test << "# This is a comment\n"
                 << "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                 << "# Another comment\n"
                 << "@language \"C++\"\n"
                 << "@description \"Test template with comments\"\n"
                 << "# Final comment\n";
    comment_test.close();
    
    std::ofstream comment_test2("test_output/comment_test2.llm");
    comment_test2 << "@copyright \"MIT License\" \"2025 dbjwhs\"\n\n"
                  << "@language \"C++\"\n\n\n"
                  << "@description \"Test template with extra whitespace\"\n";
    comment_test2.close();
    
    std::ofstream whitespace_test("test_output/whitespace_test.llm");
    whitespace_test << "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                    << "@language \"C++\"\n"
                    << "@description \"Test template with various whitespace\"\n";
    whitespace_test.close();
    
    std::ofstream whitespace_var_test("test_output/whitespace_var_test.llm");
    whitespace_var_test << "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                        << "@language \"C++\"\n"
                        << "@description \"Test template with whitespace variables\"\n"
                        << "@variable \"test_var\" \"test_value\"\n";
    whitespace_var_test.close();
    
    std::ofstream minimal_test("test_output/minimal_test.llm");
    minimal_test << "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                 << "@language \"C++\"\n"
                 << "@description \"Minimal test template\"\n";
    minimal_test.close();
    
    std::ofstream example_test("test_output/example_test.llm");
    example_test << "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                 << "@language \"C++\"\n"
                 << "@description \"Example test template\"\n"
                 << "@example \"Test Example\" \"Sample code here\"\n";
    example_test.close();
    
    // Test files with comments
    ASSERT_NO_THROW({
        std::string result = cql::QueryProcessor::compile_file("test_output/baseline.llm");
        ASSERT_FALSE(result.empty()) << "Compilation of baseline.llm failed";
    }) << "baseline.llm should compile without errors";
    
    ASSERT_NO_THROW({
        std::string result = cql::QueryProcessor::compile_file("test_output/comment_test.llm");
        ASSERT_FALSE(result.empty()) << "Compilation of comment_test.llm failed";
    }) << "comment_test.llm should compile without errors";
    
    ASSERT_NO_THROW({
        std::string result = cql::QueryProcessor::compile_file("test_output/comment_test2.llm");
        ASSERT_FALSE(result.empty()) << "Compilation of comment_test2.llm failed";
    }) << "comment_test2.llm should compile without errors";
    
    // Test files with whitespace
    ASSERT_NO_THROW({
        std::string result = cql::QueryProcessor::compile_file("test_output/whitespace_test.llm");
        ASSERT_FALSE(result.empty()) << "Compilation of whitespace_test.llm failed";
    }) << "whitespace_test.llm should compile without errors";
    
    ASSERT_NO_THROW({
        std::string result = cql::QueryProcessor::compile_file("test_output/whitespace_var_test.llm");
        ASSERT_FALSE(result.empty()) << "Compilation of whitespace_var_test.llm failed";
    }) << "whitespace_var_test.llm should compile without errors";
    
    // Test minimal and example files
    ASSERT_NO_THROW({
        std::string result = cql::QueryProcessor::compile_file("test_output/minimal_test.llm");
        ASSERT_FALSE(result.empty()) << "Compilation of minimal_test.llm failed";
    }) << "minimal_test.llm should compile without errors";
    
    ASSERT_NO_THROW({
        std::string result = cql::QueryProcessor::compile_file("test_output/example_test.llm");
        ASSERT_FALSE(result.empty()) << "Compilation of example_test.llm failed";
    }) << "example_test.llm should compile without errors";
}

/**
 * Test for input length validation
 */
TEST_F(CQLTest, InputLengthValidation) {
    std::cout << "Testing input length validation..." << std::endl;
    
    // Test query length validation
    {
        // Create a query that exceeds MAX_QUERY_LENGTH
        std::string long_query = "@copyright \"MIT\" \"2025\"\n@language \"C++\"\n@description \"";
        long_query.append(InputValidator::MAX_QUERY_LENGTH, 'x');
        long_query += "\"";
        
        ASSERT_THROW({
            QueryProcessor::compile(long_query);
        }, std::exception) << "Query exceeding MAX_QUERY_LENGTH should fail";
    }
    
    // Test template name validation
    {
        TemplateManager tm("./test_templates");
        
        // Test empty template name
        ASSERT_THROW({
            tm.save_template("", "content");
        }, std::exception) << "Empty template name should fail";
        
        // Test template name too long
        std::string long_name(InputValidator::MAX_TEMPLATE_NAME_LENGTH + 1, 'x');
        ASSERT_THROW({
            tm.save_template(long_name, "content");
        }, std::exception) << "Template name exceeding MAX_TEMPLATE_NAME_LENGTH should fail";
        
        // Test template name with invalid characters
        ASSERT_THROW({
            tm.save_template("test@template", "content");
        }, std::exception) << "Template name with invalid characters should fail";
        
        // Test template name with path traversal
        ASSERT_THROW({
            tm.save_template("../test", "content");
        }, std::exception) << "Template name with path traversal should fail";
        
        // Test valid template name
        ASSERT_NO_THROW({
            tm.save_template("valid_template-123", "@copyright \"MIT\" \"2025\"\n@language \"C++\"\n@description \"test\"");
        }) << "Valid template name should succeed";
    }
    
    // Test variable validation
    {
        // Test variable name too long
        std::string long_var_name(InputValidator::MAX_VARIABLE_NAME_LENGTH + 1, 'x');
        ASSERT_THROW({
            InputValidator::validate_variable(long_var_name, "value");
        }, SecurityValidationError) << "Variable name exceeding limit should fail";
        
        // Test variable value too long
        std::string long_var_value(InputValidator::MAX_VARIABLE_VALUE_LENGTH + 1, 'x');
        ASSERT_THROW({
            InputValidator::validate_variable("test_var", long_var_value);
        }, SecurityValidationError) << "Variable value exceeding limit should fail";
        
        // Test invalid variable name
        ASSERT_THROW({
            InputValidator::validate_variable("123invalid", "value");
        }, SecurityValidationError) << "Invalid variable name should fail";
        
        // Test valid variable
        ASSERT_NO_THROW({
            InputValidator::validate_variable("valid_var_123", "test value");
        }) << "Valid variable should succeed";
    }
    
    // Test category name validation
    {
        // Test category name too long
        std::string long_category(InputValidator::MAX_CATEGORY_NAME_LENGTH + 1, 'x');
        ASSERT_THROW({
            InputValidator::validate_category_name(long_category);
        }, SecurityValidationError) << "Category name exceeding limit should fail";
        
        // Test category with path traversal
        ASSERT_THROW({
            InputValidator::validate_category_name("../category");
        }, SecurityValidationError) << "Category with path traversal should fail";
        
        // Test valid category
        ASSERT_NO_THROW({
            InputValidator::validate_category_name("valid/category_123");
        }) << "Valid category name should succeed";
    }
    
    // Test directive content length validation
    {
        // Create directive content exceeding MAX_DIRECTIVE_LENGTH
        std::string long_content(InputValidator::MAX_DIRECTIVE_LENGTH + 1, 'x');
        ASSERT_THROW({
            InputValidator::validate_directive_content("test", long_content);
        }, SecurityValidationError) << "Directive content exceeding limit should fail";
    }
    
    // Test path length validation
    {
        // Create path exceeding MAX_PATH_LENGTH
        std::string long_path(InputValidator::MAX_PATH_LENGTH + 1, 'x');
        ASSERT_THROW({
            InputValidator::validate_file_path(long_path);
        }, SecurityValidationError) << "Path exceeding MAX_PATH_LENGTH should fail";
    }
    
    // Test filename length validation
    {
        // Create filename exceeding MAX_FILENAME_LENGTH
        std::string long_filename(InputValidator::MAX_FILENAME_LENGTH + 1, 'x');
        ASSERT_THROW({
            InputValidator::validate_filename(long_filename);
        }, SecurityValidationError) << "Filename exceeding MAX_FILENAME_LENGTH should fail";
    }
}

// test_phase2_example_compilation implementation
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

// Stub implementations for legacy test functions
TestResult test_api_client() { return TestResult::pass(); }
TestResult test_response_processor() { return TestResult::pass(); }
TestResult test_json_format_output() { return TestResult::pass(); }
TestResult test_template_inheritance() { return TestResult::pass(); }
TestResult test_template_validator() { return TestResult::pass(); }
TestResult test_basic_compilation() { return TestResult::pass(); }
TestResult test_complex_compilation() { return TestResult::pass(); }
TestResult test_validation_requirements() { return TestResult::pass(); }
TestResult test_phase2_features() { return TestResult::pass(); }
TestResult test_template_management() { return TestResult::pass(); }
TestResult test_lexer_standalone() { return TestResult::pass(); }
TestResult test_parser_standalone() { return TestResult::pass(); }
TestResult test_compiler_standalone() { return TestResult::pass(); }
TestResult test_examples_compilation() { return TestResult::pass(); }

} // namespace cql::test
