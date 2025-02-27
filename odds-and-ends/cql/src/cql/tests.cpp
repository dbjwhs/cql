// MIT License
// Copyright (c) 2025 dbjwhs

#include <cassert>
#include <iostream>
#include <sstream>
#include "../../include/cql/cql.hpp"
#include "../../../headers/project_utils.hpp"

namespace cql::test {

// Test suite for the lexer
void test_lexer() {
    Logger::getInstance().log(LogLevel::INFO, "Running lexer tests...");

    // Test basic tokenization
    {
        std::string input = "@language \"C++\"\n@description \"implement a stack\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::LANGUAGE);
        Logger::getInstance().log(LogLevel::DEBUG, "Token 1: ", token1->to_string());

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING && token2->m_value == "C++");
        Logger::getInstance().log(LogLevel::DEBUG, "Token 2: ", token2->to_string());

        auto token3 = lexer.next_token();
        assert(token3 && token3->m_type == TokenType::NEWLINE);
        Logger::getInstance().log(LogLevel::DEBUG, "Token 3: ", token3->to_string());

        auto token4 = lexer.next_token();
        assert(token4 && token4->m_type == TokenType::DESCRIPTION);

        auto token5 = lexer.next_token();
        assert(token5 && token5->m_type == TokenType::STRING && token5->m_value == "implement a stack");

        auto token6 = lexer.next_token();
        assert(!token6);
    }

    // Test string escape sequences
    {
        std::string input = "@language \"C++\\n with newline\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::LANGUAGE);

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING);
        assert(token2->m_value == "C++\n with newline");
    }

    // Test error handling - unterminated string
    try {
        std::string input = "@language \"unterminated string";
        Lexer lexer(input);
        lexer.next_token(); // @language
        lexer.next_token(); // should throw
        assert(false && "Expected exception for unterminated string");
    } catch (const LexerError& e) {
        Logger::getInstance().log(LogLevel::DEBUG, "Expected exception: ", e.what());
    }

    // Test new tokens
    {
        std::string input = "@dependency \"boost::asio\"\n@performance \"latency < 5ms\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::DEPENDENCY);

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING && token2->m_value == "boost::asio");

        auto token3 = lexer.next_token();
        assert(token3 && token3->m_type == TokenType::NEWLINE);

        auto token4 = lexer.next_token();
        assert(token4 && token4->m_type == TokenType::PERFORMANCE);

        auto token5 = lexer.next_token();
        assert(token5 && token5->m_type == TokenType::STRING && token5->m_value == "latency < 5ms");
    }

    // Test copyright token
    {
        std::string input = "@copyright \"MIT License\" \"2025 dbjwhs\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::COPYRIGHT);

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING && token2->m_value == "MIT License");

        auto token3 = lexer.next_token();
        assert(token3 && token3->m_type == TokenType::STRING && token3->m_value == "2025 dbjwhs");
    }

    Logger::getInstance().log(LogLevel::INFO, "Lexer tests passed!");
}

// Test suite for the parser
void test_parser() {
    Logger::getInstance().log(LogLevel::INFO, "Running parser tests...");

    // Test basic parsing
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
        auto nodes = parser.parse();

        assert(nodes.size() == 5);
        Logger::getInstance().log(LogLevel::DEBUG, "Parsed ", nodes.size(), " nodes");

        // Verify node types
        auto* code_request = dynamic_cast<CodeRequestNode*>(nodes[0].get());
        assert(code_request && code_request->language() == "C++");
        assert(code_request->description() == "implement a thread-safe queue");

        auto* context = dynamic_cast<ContextNode*>(nodes[1].get());
        assert(context && context->context() == "Using Modern C++ features");

        auto* test = dynamic_cast<TestNode*>(nodes[2].get());
        assert(test && test->test_cases().size() == 1);
        assert(test->test_cases()[0] == "Test empty queue");

        auto* dependency = dynamic_cast<DependencyNode*>(nodes[3].get());
        assert(dependency && dependency->dependencies().size() == 1);
        assert(dependency->dependencies()[0] == "std::mutex");

        auto* performance = dynamic_cast<PerformanceNode*>(nodes[4].get());
        assert(performance && performance->requirement() == "Handle 1M operations per second");
    }

    // Test error handling - missing description
    try {
        std::string input = "@language \"C++\"";
        Parser parser(input);
        parser.parse();
        assert(false && "Expected exception for missing description");
    } catch (const ParserError& e) {
        Logger::getInstance().log(LogLevel::DEBUG, "Expected exception: ", e.what());
    }

    // Test error handling - invalid token
    try {
        std::string input = "@invalid \"test\"";
        Parser parser(input);
        parser.parse();
        assert(false && "Expected exception for invalid token");
    } catch (const LexerError& e) {
        Logger::getInstance().log(LogLevel::DEBUG, "Expected exception: ", e.what());
    }

    Logger::getInstance().log(LogLevel::INFO, "Parser tests passed!");
}

// Test suite for the compiler
void test_compiler() {
    Logger::getInstance().log(LogLevel::INFO, "Running compiler tests...");

    // Test basic compilation
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

        // Verify the compiled query contains expected sections
        assert(util::contains(result, "Please generate C++ code that:"));
        assert(util::contains(result, "implement a thread-safe queue"));
        assert(util::contains(result, "Context:"));
        assert(util::contains(result, "Using Modern C++ features"));
        assert(util::contains(result, "Please include tests for the following cases:"));
        assert(util::contains(result, "Test empty queue"));

        Logger::getInstance().log(LogLevel::DEBUG, "Compiled query: ", result);
    }
    
    // Test new Phase 2 directives
    {
        std::string input = R"(
            @language "C++"
            @description "implement a sorting algorithm"
            @context "For educational purposes"
            
            @architecture "Standalone module"
            @constraint "Must work with custom comparators"
            @complexity "O(n log n) average case complexity"
            @security "Input validation to prevent integer overflow"
            
            @variable "algorithm_name" "QuickSort"
            
            @example "Basic Usage" "
            std::vector<int> data = {5, 2, 9, 1, 5, 6};
            ${algorithm_name}(data.begin(), data.end());
            // data is now sorted
            "
            
            @test "Basic sorting test"
            @test "Empty collection test"
            @test "Already-sorted collection test"
        )";

        Parser parser(input);
        auto nodes = parser.parse();

        QueryCompiler compiler;
        for (const auto& node : nodes) {
            node->accept(compiler);
        }

        std::string result = compiler.get_compiled_query();

        // Verify the compiled query contains expected new sections
        assert(util::contains(result, "Architecture Requirements:"));
        assert(util::contains(result, "Standalone module"));
        assert(util::contains(result, "Constraints:"));
        assert(util::contains(result, "Must work with custom comparators"));
        assert(util::contains(result, "Algorithmic Complexity Requirements:"));
        assert(util::contains(result, "O(n log n) average case complexity"));
        assert(util::contains(result, "Security Requirements:"));
        assert(util::contains(result, "Input validation to prevent integer overflow"));
        
        // Check example section
        assert(util::contains(result, "Example - Basic Usage:"));
        
        // Check variable interpolation
        assert(util::contains(result, "QuickSort(data.begin(), data.end());"));
        
        Logger::getInstance().log(LogLevel::DEBUG, "Compiled Phase 2 query: ", result);
    }
    
    // Test format directives
    {
        std::string input = R"(
            @language "C++"
            @description "simple hello world"
            @model "claude-3-sonnet"
            @format "json"
        )";

        Parser parser(input);
        auto nodes = parser.parse();

        QueryCompiler compiler;
        for (const auto& node : nodes) {
            node->accept(compiler);
        }

        std::string result = compiler.get_compiled_query();

        // Verify JSON output format
        assert(util::contains(result, "\"model\": \"claude-3-sonnet\""));
        assert(util::contains(result, "\"format\": \"json\""));
        
        Logger::getInstance().log(LogLevel::DEBUG, "JSON formatted query: ", result);
    }

    // Test extended compilation with new node types
    {
        std::string input = R"(
            @language "C++"
            @description "implement a real-time data processor"
            @context "Embedded system environment"
            @dependency "boost::asio"
            @performance "Process 10k messages/second"
            @test "Test throughput under load"
        )";

        Parser parser(input);
        auto nodes = parser.parse();

        QueryCompiler compiler;
        for (const auto& node : nodes) {
            node->accept(compiler);
        }

        std::string result = compiler.get_compiled_query();

        // Verify the compiled query contains expected sections
        assert(util::contains(result, "Please generate C++ code that:"));
        assert(util::contains(result, "implement a real-time data processor"));
        assert(util::contains(result, "Context:"));
        assert(util::contains(result, "Embedded system environment"));
        assert(util::contains(result, "Dependencies:"));
        assert(util::contains(result, "boost::asio"));
        assert(util::contains(result, "Performance Requirements:"));
        assert(util::contains(result, "Process 10k messages/second"));

        Logger::getInstance().log(LogLevel::DEBUG, "Compiled query: ", result);
    }

    // Test compiler copyright
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

        // Verify the compiled query contains expected sections
        assert(util::contains(result, "Please include the following copyright header"));
        assert(util::contains(result, "MIT License"));
        assert(util::contains(result, "Copyright (c) 2025 dbjwhs"));
        assert(util::contains(result, "Please generate C++ code that:"));

        Logger::getInstance().log(LogLevel::DEBUG, "Compiled query with copyright: ", result);
    }

    Logger::getInstance().log(LogLevel::INFO, "Compiler tests passed!");
}

// Showcase example queries
void query_examples() {
    std::cout << "\n=== Query Examples ===" << std::endl;
    Logger::getInstance().log(LogLevel::INFO, "\n=== Query Examples ===");

    // Example 1: simple function query
    std::cout << "\nExample 1 - Simple Function:" << std::endl;
    Logger::getInstance().log(LogLevel::INFO, "\nExample 1 - Simple Function:");
    std::string simple_query =
        "@language \"C++\"\n"
        "@description \"implement a string reverse function\"\n"
        "@context \"Using string_view for efficiency\"\n"
        "@test \"Empty string\"\n"
        "@test \"Single character\"\n"
        "@test \"Multiple characters\"\n";

    std::cout << "Input DSL:\n" << simple_query << std::endl;
    
    try {
        std::string result = QueryProcessor::compile(simple_query);
        Logger::getInstance().log(LogLevel::INFO, "Input DSL:\n", simple_query);
        std::cout << "\n=== Compiled Query ===\n\n" << result << "\n===================" << std::endl;
        Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR, "Error: ", e.what());
    }

    // Example 2: class implementation query
    std::cout << "\nExample 2 - Class Implementation:" << std::endl;
    Logger::getInstance().log(LogLevel::INFO, "\nExample 2 - Class Implementation:");
    std::string class_query =
        "@language \"C++\"\n"
        "@description \"implement a thread-safe queue class with a maximum size\"\n"
        "@context \"Using C++20 features and RAII principles\"\n"
        "@context \"Must be exception-safe\"\n"
        "@dependency \"std::mutex, std::condition_variable\"\n"
        "@performance \"Support 100k operations per second\"\n"
        "@test \"Test concurrent push operations\"\n"
        "@test \"Test concurrent pop operations\"\n"
        "@test \"Test boundary conditions (empty/full)\"\n"
        "@test \"Test exception safety guarantees\"\n";

    std::cout << "Input DSL:\n" << class_query << std::endl;
    
    try {
        std::string result = QueryProcessor::compile(class_query);
        Logger::getInstance().log(LogLevel::INFO, "Input DSL:\n", class_query);
        std::cout << "\n=== Compiled Query ===\n\n" << result << "\n===================" << std::endl;
        Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR, "Error: ", e.what());
    }

    // Example 3: with copyright and license
    std::cout << "\nExample 3 - With Copyright and License:" << std::endl;
    Logger::getInstance().log(LogLevel::INFO, "\nExample 3 - With Copyright and License:");
    std::string copyright_query =
        "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
        "@language \"C++\"\n"
        "@description \"implement a binary search tree\"\n"
        "@context \"Modern C++ implementation\"\n"
        "@test \"Insert elements\"\n"
        "@test \"Delete elements\"\n"
        "@test \"Find elements\"\n";

    std::cout << "Input DSL:\n" << copyright_query << std::endl;
    
    try {
        std::string result = QueryProcessor::compile(copyright_query);
        Logger::getInstance().log(LogLevel::INFO, "Input DSL:\n", copyright_query);
        std::cout << "\n=== Compiled Query ===\n\n" << result << "\n===================" << std::endl;
        Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR, "Error: ", e.what());
    }
}

// Run all tests
// Test suite for the validator
void test_validator() {
    Logger::getInstance().log(LogLevel::INFO, "Running validator tests...");
    
    // Test missing required directives
    {
        std::string input = 
            "@context \"Should fail because language and description are missing\"\n";
        
        Parser parser(input);
        auto nodes = parser.parse();
        
        QueryValidator validator;
        auto issues = validator.validate(nodes);
        
        // Should have two errors for missing @language and @description
        assert(issues.size() >= 2);
        assert(std::any_of(issues.begin(), issues.end(), [](const ValidationIssue& issue) {
            return issue.severity == ValidationSeverity::ERROR && 
                   issue.message.find("@LANGUAGE") != std::string::npos;
        }));
        assert(std::any_of(issues.begin(), issues.end(), [](const ValidationIssue& issue) {
            return issue.severity == ValidationSeverity::ERROR && 
                   issue.message.find("@DESCRIPTION") != std::string::npos;
        }));
        
        Logger::getInstance().log(LogLevel::DEBUG, "Validation correctly detected missing required directives");
    }
    
    // Test dependency rule
    {
        std::string input = 
            "@language \"C++\"\n"
            "@description \"Test dependency rule\"\n"
            "@architecture \"Microservices\"\n";
        
        Parser parser(input);
        auto nodes = parser.parse();
        
        QueryValidator validator;
        auto issues = validator.validate(nodes);
        
        // Should have a warning about architecture being used without context
        assert(std::any_of(issues.begin(), issues.end(), [](const ValidationIssue& issue) {
            return issue.severity == ValidationSeverity::WARNING && 
                   issue.message.find("@ARCHITECTURE") != std::string::npos &&
                   issue.message.find("@CONTEXT") != std::string::npos;
        }));
        
        Logger::getInstance().log(LogLevel::DEBUG, "Validation correctly detected dependency rule violation");
    }
    
    // Test custom rule for missing tests
    {
        std::string input = 
            "@language \"C++\"\n"
            "@description \"Should warn about missing tests\"\n";
        
        Parser parser(input);
        auto nodes = parser.parse();
        
        QueryValidator validator;
        auto issues = validator.validate(nodes);
        
        // Should have a warning about missing tests
        assert(std::any_of(issues.begin(), issues.end(), [](const ValidationIssue& issue) {
            return issue.severity == ValidationSeverity::WARNING && 
                   issue.message.find("test") != std::string::npos;
        }));
        
        Logger::getInstance().log(LogLevel::DEBUG, "Validation correctly detected missing tests");
    }
    
    // Test valid query
    {
        std::string input = 
            "@language \"C++\"\n"
            "@description \"Valid query with all required elements\"\n"
            "@context \"Development environment\"\n"
            "@test \"Basic functionality\"\n";
        
        std::cout << "Input query for validation test:" << std::endl << input << std::endl;
        
        Parser parser(input);
        auto nodes = parser.parse();
        
        std::cout << "Parsed " << nodes.size() << " nodes:" << std::endl;
        for (const auto& node : nodes) {
            if (dynamic_cast<const CodeRequestNode*>(node.get())) {
                auto* code_node = dynamic_cast<const CodeRequestNode*>(node.get());
                std::cout << "  CodeRequestNode: language=" << code_node->language() 
                          << ", description=" << code_node->description() << std::endl;
            } else if (dynamic_cast<const ContextNode*>(node.get())) {
                auto* ctx_node = dynamic_cast<const ContextNode*>(node.get());
                std::cout << "  ContextNode: " << ctx_node->context() << std::endl;
            } else if (dynamic_cast<const TestNode*>(node.get())) {
                auto* test_node = dynamic_cast<const TestNode*>(node.get());
                std::cout << "  TestNode: " << test_node->test_cases()[0] << std::endl;
            } else {
                std::cout << "  Other node type" << std::endl;
            }
        }
        
        QueryValidator validator;
        auto issues = validator.validate(nodes);
        
        // Print the issues for debugging
        for (const auto& issue : issues) {
            std::string severity_str;
            switch (issue.severity) {
                case ValidationSeverity::INFO: severity_str = "INFO"; break;
                case ValidationSeverity::WARNING: severity_str = "WARNING"; break;
                case ValidationSeverity::ERROR: severity_str = "ERROR"; break;
            }
            
            std::cout << "Validation " << severity_str << ": " << issue.message << std::endl;
        }
        
        // Should have no errors (maybe warnings, but no errors)
        bool has_errors = std::any_of(issues.begin(), issues.end(), [](const ValidationIssue& issue) {
            return issue.severity == ValidationSeverity::ERROR;
        });
        
        if (has_errors) {
            std::cout << "ERROR: Valid query was flagged with errors" << std::endl;
        } else {
            Logger::getInstance().log(LogLevel::DEBUG, "Validation correctly approved valid query");
        }
        
        // Allow warnings, but no errors
        assert(!has_errors);
    }
    
    Logger::getInstance().log(LogLevel::INFO, "Validator tests passed!");
}

// Test template manager
void test_template_manager() {
    std::cout << "Running template manager tests..." << std::endl;
    Logger::getInstance().log(LogLevel::INFO, "Running template manager tests...");

    try {
        // Create a temporary directory for testing
        std::string temp_dir = "./temp_test_templates";
        TemplateManager manager(temp_dir);
        
        // Test saving a template
        std::string test_template = 
            "@language \"C++\"\n"
            "@description \"test template\"\n"
            "@variable \"class_name\" \"DefaultClass\"\n"
            "@context \"Using ${class_name} for implementation\"\n"
            "@test \"Test ${class_name} constructor\"\n";
        
        manager.save_template("test_template", test_template);
        
        // Test listing templates
        auto templates = manager.list_templates();
        assert(!templates.empty());
        assert(templates[0] == "test_template.cql");
        
        // Test loading a template
        std::string loaded_template = manager.load_template("test_template");
        assert(loaded_template == test_template);
        
        // Test getting template metadata
        auto metadata = manager.get_template_metadata("test_template");
        assert(metadata.name == "test_template");
        assert(metadata.description == "test template");
        assert(metadata.variables.size() == 1);
        assert(metadata.variables[0] == "class_name");
        
        // Test instantiating a template with variables
        std::map<std::string, std::string> variables{{"class_name", "TestClass"}};
        std::string instantiated = manager.instantiate_template("test_template", variables);
        assert(instantiated.find("@variable \"class_name\" \"TestClass\"") != std::string::npos);
        
        // Test deleting a template
        bool deleted = manager.delete_template("test_template");
        assert(deleted);
        templates = manager.list_templates();
        assert(templates.empty());
        
        // Test categories
        bool created = manager.create_category("test_category");
        assert(created);
        auto categories = manager.list_categories();
        assert(!categories.empty());
        assert(categories[0] == "test_category");
        
        // Clean up
        std::filesystem::remove_all(temp_dir);
        
        std::cout << "Template manager tests passed!" << std::endl;
        Logger::getInstance().log(LogLevel::INFO, "Template manager tests passed!");
    } catch (const std::exception& e) {
        std::cerr << "Template manager test failed: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR, "Template manager test failed: ", e.what());
        throw;
    }
}

void run_tests() {
    std::cout << "Starting CQL test suite" << std::endl;
    Logger::getInstance().log(LogLevel::INFO, "Starting CQL test suite");

    try {
        std::cout << "Running lexer tests..." << std::endl;
        test_lexer();
        std::cout << "Running parser tests..." << std::endl;
        test_parser();
        std::cout << "Running compiler tests..." << std::endl;
        test_compiler();
        std::cout << "Running validator tests..." << std::endl;
        test_validator();
        std::cout << "Running template manager tests..." << std::endl;
        test_template_manager();

        std::cout << "All tests passed!" << std::endl;
        Logger::getInstance().log(LogLevel::INFO, "All tests passed!");
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR, "Test failed: ", e.what());
        throw; // Rethrow to signal test failure
    }
}

} // namespace cql::test