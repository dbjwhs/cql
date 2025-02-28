// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_HPP
#define CQL_HPP

/**
 * claude query language (cql) - main header
 * 
 * the claude query language was developed in 2025 as a domain-specific language to formalize
 * and standardize how developers craft queries for large language models (llms), specifically
 * anthropic's claude. it follows the compiler pattern where a high-level representation (the cql)
 * is translated into a more detailed and structured query string.
 * 
 * this file provides the main public interface for the cql library.
 */

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <fstream>
#include <optional>
#include <map>

#include "nodes.hpp"
#include "visitor.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "validator.hpp"
#include "template_manager.hpp"
#include "template_validator.hpp"
#include "template_validator_schema.hpp"

namespace cql {

/**
 * file and string utility functions
 */
namespace util {
    // read the contents of a file
    std::string read_file(const std::string& filepath);
    
    // write content to a file
    void write_file(const std::string& filepath, const std::string& content);
    
    // check if a string contains a substring
    bool contains(const std::string& str, const std::string& substr);
    
    // extract regex matches from text using a pattern
    // returns a vector of matched groups for each match
    std::vector<std::vector<std::string>> extract_regex_matches(
        const std::string& content, 
        const std::string& pattern,
        size_t expected_groups = 0
    );
    
    // extract string values that match a specific regex group
    // returns a set of unique matched values from the specified group
    std::set<std::string> extract_regex_group_values(
        const std::string& content, 
        const std::string& pattern,
        size_t group_index = 1
    );
}

/**
 * Main CQL processor class
 * Provides a simplified interface for compiling CQL queries
 * 
 * The compilation process involves:
 * 1. Parsing the query text into an AST
 * 2. Validating the AST structure and content
 * 3. Compiling the validated AST into a formatted query
 * 
 * The improved implementation prioritizes validation over syntax,
 * ensuring that users receive the most relevant error messages first.
 */
class QueryProcessor {
public:
    /**
     * Compile a CQL string to a structured query
     * @param query_str The CQL query string to compile
     * @return The compiled query as a string
     * @throws std::runtime_error for validation or parsing errors
     */
    static std::string compile(const std::string_view query_str);
    
    /**
     * Compile a CQL file to a structured query
     * @param filepath Path to the CQL file
     * @return The compiled query as a string
     * @throws std::runtime_error for file I/O, validation, or parsing errors
     */
    static std::string compile_file(const std::string& filepath);
    
    /**
     * Save a compiled query to a file
     * @param query_str The CQL query string to compile
     * @param filepath Path to save the compiled output
     * @throws std::runtime_error for file I/O, validation, or parsing errors
     */
    static void save_compiled(const std::string_view query_str, const std::string& filepath);
    
    /**
     * Compile a template with variable substitutions
     * @param template_name Name of the template to compile
     * @param variables Map of variable names to values for substitution
     * @return The compiled query as a string
     * @throws std::runtime_error for template loading, validation, or parsing errors
     */
    static std::string compile_template(const std::string& template_name, 
                                     const std::map<std::string, std::string>& variables);
};

/**
 * test framework
 */
namespace test {
    /**
     * class to represent test results
     */
    class TestResult {
    public:
        /**
         * constructor for a passing test result
         */
        static TestResult pass();
        
        /**
         * constructor for a failing test result
         * @param error_message the error message describing the failure
         * @param file_name the source file where the failure occurred
         * @param line_number the line number where the failure occurred
         */
        static TestResult fail(const std::string& error_message, 
                               const std::string& file_name = "", 
                               int line_number = 0);
        
        /**
         * check if the test passed
         * @return true if the test passed, false otherwise
         */
        bool passed() const;
        
        /**
         * get the error message for a failing test
         * @return the error message
         */
        const std::string& get_error_message() const;
        
        /**
         * get the source file where the failure occurred
         * @return the source file name
         */
        const std::string& get_file_name() const;
        
        /**
         * get the line number where the failure occurred
         * @return the line number
         */
        int get_line_number() const;
        
    private:
        bool m_passed;
        std::string m_error_message;
        std::string m_file_name;
        int m_line_number;
        
        // private constructor used by static factory methods
        TestResult(bool passed, const std::string& error_message = "",
                  const std::string& file_name = "", int line_number = 0);
    };
    
    /**
     * run all tests
     * @param fail_fast if true, stop testing after the first failure
     * @return true if all tests passed, false otherwise
     */
    bool run_tests(bool fail_fast = true);
    
    /**
     * test the lexer
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_lexer();
    
    /**
     * test the parser
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_parser();
    
    /**
     * test the compiler
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_compiler();
    
    /**
     * test basic compilation
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_basic_compilation();
    
    /**
     * test complex compilation
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_complex_compilation();
    
    /**
     * test validation requirements
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_validation_requirements();
    
    /**
     * test phase 2 features
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_phase2_features();
    
    /**
     * test the template manager
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_template_manager();
    
    /**
     * test template management
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_template_management();
    
    /**
     * test template inheritance
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_template_inheritance();
    
    /**
     * test template validator
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_template_validator();
    
    /**
     * example queries
     * @return testresult indicating pass/fail with error details
     */
    TestResult query_examples();
    
    /**
     * test the phase 2 comprehensive example compilation
     * @return testresult indicating pass/fail with error details
     */
    TestResult test_phase2_example_compilation();
}

/**
 * interactive cli functions
 */
namespace cli {
    // run the interactive cli
    void run_cli();
    
    // process a query file
    bool process_file(const std::string& input_file, const std::string& output_file);
}

} // namespace cql

#endif // cql_hpp
