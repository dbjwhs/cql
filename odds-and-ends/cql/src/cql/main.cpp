// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include "../../include/cql/cql.hpp"
#include "../../../headers/project_utils.hpp"

int main(int argc, char* argv[]) {
    // Initialize logger
    auto& logger = Logger::getInstance();
    std::cout << "Starting CQL Compiler v1.0..." << std::endl;
    logger.log(LogLevel::INFO, "Claude Query Language (CQL) Compiler v1.0");

    try {
        std::cout << "Parsing command line arguments..." << std::endl;
        // Parse command line arguments
        if (argc > 1) {
            std::string arg1 = argv[1];
            std::cout << "Received argument: " << arg1 << std::endl;

            if (arg1 == "--help" || arg1 == "-h") {
                // Show help information
                std::cout << "Claude Query Language (CQL) Compiler v1.0\n"
                          << "Usage: cql [OPTIONS] [INPUT_FILE] [OUTPUT_FILE]\n\n"
                          << "Options:\n"
                          << "  --help, -h        Show this help information\n"
                          << "  --test, -t        Run the test suite\n"
                          << "  --examples, -e    Show example queries\n"
                          << "  --interactive, -i Run in interactive mode\n"
                          << "  --copyright       Show copyright example\n\n"
                          << "If INPUT_FILE is provided, it will be processed as a CQL query.\n"
                          << "If OUTPUT_FILE is also provided, the compiled query will be written to it.\n";
            } else if (arg1 == "--test" || arg1 == "-t") {
                // Run the test suite
                cql::test::run_tests();
            } else if (arg1 == "--examples" || arg1 == "-e") {
                // Show example queries
                cql::test::query_examples();
            } else if (arg1 == "--interactive" || arg1 == "-i") {
                // Run in interactive mode
                cql::cli::run_cli();
            } else if (arg1 == "--copyright") {
                // Show an example of copyright usage
                std::string copyright_example =
                    "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                    "@language \"C++\"\n"
                    "@description \"implement a thread-safe queue\"\n";

                logger.log(LogLevel::INFO, "Copyright Example DSL:\n", copyright_example);
                
                std::string result = cql::QueryProcessor::compile(copyright_example);
                logger.log(LogLevel::INFO, "\n=== Compiled Query with Copyright ===\n\n", 
                          result, "\n===================");
            } else {
                // Assume it's an input file
                std::string output_file;
                if (argc > 2) {
                    output_file = argv[2];
                }

                if (!cql::cli::process_file(arg1, output_file)) {
                    return 1;
                }
            }
        } else {
            // No arguments, run comprehensive tests and examples
            std::cout << "Running in default mode - tests and examples" << std::endl;
            logger.log(LogLevel::INFO, "Running in default mode - tests and examples");
            std::cout << "Running tests..." << std::endl;
            cql::test::run_tests();
            std::cout << "Running query examples..." << std::endl;
            cql::test::query_examples();

            // Example query - note the careful spacing and newlines
            std::string query =
                "@language \"C++\"\n"
                "@description \"implement a thread-safe queue with a maximum size\"\n"
                "@context \"Using C++20 features and RAII principles\"\n"
                "@test \"Test concurrent push operations\"\n"
                "@test \"Test concurrent pop operations\"\n"
                "@test \"Test boundary conditions\"\n";

            std::cout << "\nDefault example:" << std::endl;
            std::cout << "Input query:\n" << query << std::endl;
            logger.log(LogLevel::INFO, "\nDefault example:");
            logger.log(LogLevel::INFO, "Input query:\n", query);

            std::cout << "Compiling default example..." << std::endl;
            std::string result = cql::QueryProcessor::compile(query);
            std::cout << "\n=== Compiled Query ===\n\n" << result << "\n===================" << std::endl;
            logger.log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
        }
    } catch (const std::exception& e) {
        logger.log(LogLevel::ERROR, "Fatal error: ", e.what());
        return 1;
    }
    
    return 0;
}