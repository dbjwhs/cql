// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace cql {

/**
 * @class CommandLineHandler
 * @brief Handles command-line argument parsing and option management
 * 
 * This class encapsulates all command-line parsing logic,
 * providing a clean interface for option detection and value extraction.
 */
class CommandLineHandler {
public:
    /**
     * @brief Construct a command line handler
     * @param argc Argument count from main()
     * @param argv Argument array from main()
     */
    CommandLineHandler(int argc, char* argv[]);

    /**
     * @brief Check if a command-line option exists
     * @param option The option to check for (e.g., "--help", "-h")
     * @return true if the option exists
     */
    [[nodiscard]] bool has_option(const std::string& option) const;

    /**
     * @brief Get the value associated with an option
     * @param option The option to get the value for
     * @return Optional string containing the value if it exists
     */
    [[nodiscard]] std::optional<std::string> get_option_value(const std::string& option) const;

    /**
     * @brief Find and remove an option from the arguments
     * @param option The option to find and remove
     * @param value Reference to store the option value if found
     * @return true if the option was found and removed
     */
    bool find_and_remove_option(const std::string& option, std::string& value);

    /**
     * @brief Get positional arguments (non-option arguments)
     * @return Vector of positional arguments
     */
    [[nodiscard]] std::vector<std::string> get_positional_args() const;

    /**
     * @brief Get the program name (argv[0])
     * @return The program name
     */
    [[nodiscard]] std::string get_program_name() const;

    /**
     * @brief Get the current argument count
     * @return Current number of arguments
     */
    [[nodiscard]] int get_argc() const { return m_argc; }

    /**
     * @brief Get the current argument array
     * @return Pointer to argument array
     */
    [[nodiscard]] char** get_argv() const { return m_argv.get(); }

    /**
     * @brief Print help message
     */
    static void print_help();

private:
    int m_argc;
    std::unique_ptr<char*[]> m_argv;
    std::vector<std::string> m_args;  // Copy of arguments for easier manipulation

    /**
     * @brief Copy arguments to internal storage
     * @param argc Original argument count
     * @param argv Original argument array
     */
    void copy_arguments(int argc, char* argv[]);
};

} // namespace cql