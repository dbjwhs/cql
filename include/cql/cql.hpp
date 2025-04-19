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

// Version information
#define CQL_VERSION_MAJOR 0
#define CQL_VERSION_MINOR 1
#define CQL_VERSION_PATCH 0
#define CQL_VERSION_STRING "0.1.0"
#define CQL_BUILD_TIMESTAMP __DATE__ " " __TIME__

// Return code definitions
#define CQL_NO_ERROR 0  // Success return code
#define CQL_ERROR 1     // Error return code

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
    
    // copy content to clipboard
    bool copy_to_clipboard(const std::string& content);
    
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
    static std::string compile(std::string_view query_str);
    
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
    static void save_compiled(std::string_view query_str, const std::string& filepath);
    
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
 * Minimal test interface for examples
 * 
 * Note: Most test functionality has been moved to the dedicated test executable.
 * Only the TestResult class and query_examples function are maintained here
 * for backwards compatibility with the examples feature.
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
        [[nodiscard]] bool passed() const;
        
        /**
         * get the error message for a failing test
         * @return the error message
         */
        [[nodiscard]] const std::string& get_error_message() const;
        
        /**
         * get the source file where the failure occurred
         * @return the source file name
         */
        [[nodiscard]] const std::string& get_file_name() const;
        
        /**
         * get the line number where the failure occurred
         * @return the line number
         */
        [[nodiscard]] int get_line_number() const;
        
    private:
        bool m_passed;
        std::string m_error_message;
        std::string m_file_name;
        int m_line_number;
        
        // private constructor used by static factory methods
        explicit TestResult(bool passed, std::string  error_message = "",
                  std::string  file_name = "", int line_number = 0);
    };
    
    /**
     * example queries
     * @return testresult indicating pass/fail with error details
     */
    TestResult query_examples();
}

/**
 * interactive functions
 */
namespace cli {
    // run the interactive mode
    void run_interactive();
    
    // process a query file
    bool process_file(const std::string& input_file, const std::string& output_file);
    
    // process a submit command
    bool process_submit_command(const std::string& input_file, 
                              const std::string& output_dir,
                              const std::string& model,
                              bool overwrite,
                              bool create_dirs,
                              bool no_save = false);
}

} // namespace cql

#endif // cql_hpp
