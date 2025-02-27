// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <string>
#include "../../include/cql/cql.hpp"
#include "../../../headers/project_utils.hpp"

namespace cql::cli {

// CLI interface for interactive use
void run_cli() {
    Logger::getInstance().log(LogLevel::INFO, "CQL Interactive Mode");
    Logger::getInstance().log(LogLevel::INFO, "Type 'exit' to quit, 'help' for command list");

    std::string line;
    std::string current_query;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line == "exit" || line == "quit") {
            break;
        } else if (line == "help") {
            std::cout << "Commands:\n"
                      << "  help       - Show this help\n"
                      << "  exit/quit  - Exit the program\n"
                      << "  clear      - Clear the current query\n"
                      << "  show       - Show the current query\n"
                      << "  compile    - Compile the current query\n"
                      << "  load FILE  - Load query from file\n"
                      << "  save FILE  - Save compiled query to file\n";
        } else if (line == "clear") {
            current_query.clear();
            Logger::getInstance().log(LogLevel::INFO, "Query cleared");
        } else if (line == "show") {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "Current query is empty");
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Current query:\n", current_query);
            }
        } else if (line == "compile") {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::ERROR, "Nothing to compile");
                continue;
            }

            try {
                std::string result = QueryProcessor::compile(current_query);
                Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", 
                                         result, "\n===================");
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Compilation error: ", e.what());
            }
        } else if (line.substr(0, 5) == "load ") {
            std::string filename = line.substr(5);
            try {
                current_query = util::read_file(filename);
                Logger::getInstance().log(LogLevel::INFO, "Loaded query from ", filename);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to load file: ", e.what());
            }
        } else if (line.substr(0, 5) == "save ") {
            std::string filename = line.substr(5);
            try {
                if (current_query.empty()) {
                    Logger::getInstance().log(LogLevel::ERROR, "Nothing to save");
                    continue;
                }

                QueryProcessor::save_compiled(current_query, filename);
                Logger::getInstance().log(LogLevel::INFO, "Saved compiled query to ", filename);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to save file: ", e.what());
            }
        } else {
            // Add line to the current query
            if (!current_query.empty()) {
                current_query += "\n";
            }
            current_query += line;
        }
    }
}

// Process a query file
bool process_file(const std::string& input_file, const std::string& output_file) {
    try {
        Logger::getInstance().log(LogLevel::INFO, "Processing file: ", input_file);
        std::cout << "Processing file: " << input_file << std::endl;

        std::string result = QueryProcessor::compile_file(input_file);

        if (output_file.empty()) {
            std::cout << "\n=== Compiled Query ===\n\n" 
                     << result << "\n===================" << std::endl;
            Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", 
                                     result, "\n===================");
        } else {
            util::write_file(output_file, result);
            std::cout << "Compiled query written to " << output_file << std::endl;
            Logger::getInstance().log(LogLevel::INFO, "Compiled query written to ", output_file);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error processing file: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR, "Error processing file: ", e.what());
        return false;
    }
}

} // namespace cql::cli