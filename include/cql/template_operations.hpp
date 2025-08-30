// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <vector>
#include <map>
#include "template_manager.hpp"
#include "template_validator.hpp"

namespace cql {

/**
 * @class TemplateOperations
 * @brief Handles all template-related operations including listing, validation, and processing
 * 
 * This class encapsulates template management functionality that was previously
 * scattered throughout main.cpp, providing a clean interface for template operations.
 */
class TemplateOperations {
public:
    /**
     * @brief Construct template operations handler
     */
    TemplateOperations();

    /**
     * @brief List all available templates
     */
    static void list_templates();

    /**
     * @brief Initialize a template validator with default schema
     * @param manager Template manager to use
     * @return TemplateValidator Initialized validator
     */
    [[nodiscard]] static TemplateValidator initialize_template_validator(const TemplateManager& manager);

    /**
     * @brief Process variables for template instantiation
     * @param argc Argument count
     * @param argv Argument values
     * @param start_index Starting index in argv
     * @return std::map<std::string, std::string> Map of variable names to values
     */
    [[nodiscard]] static std::map<std::string, std::string> process_template_variables(int argc, char* argv[], int start_index);

    /**
     * @brief Check for --force flag in arguments
     * @param argc Argument count
     * @param argv Argument values
     * @param start_index Starting index in argv
     * @return bool True if --force flag is present
     */
    [[nodiscard]] static bool has_force_flag(int argc, char* argv[], int start_index);

    /**
     * @brief Handle missing variables in templates
     * @param validation_result Validation result
     * @param template_vars Template variables
     * @param variables User-provided variables
     * @return std::vector<std::string> List of missing variable names
     */
    [[nodiscard]] static std::vector<std::string> handle_missing_variables(
        const TemplateValidationResult& validation_result,
        const std::map<std::string, std::string>& template_vars,
        const std::map<std::string, std::string>& variables);

    /**
     * @brief Use a specific template with variables
     * @param argc Argument count
     * @param argv Argument values
     * @return int Return code (0 for success, 1 for error)
     */
    [[nodiscard]] static int handle_template_command(int argc, char* argv[]);

    /**
     * @brief Display validation results in a formatted manner
     * @param result Validation result
     * @param template_name Name of the template being validated
     */
    static void display_validation_results(const TemplateValidationResult& result, const std::string& template_name);

    /**
     * @brief Validate a specific template
     * @param argc Argument count
     * @param argv Argument values
     * @return int Return code (0 for success, 1 for error)
     */
    [[nodiscard]] static int handle_validate_command(int argc, char* argv[]);

    /**
     * @brief Validate all templates in a directory
     * @param templates_path Path to templates directory
     * @return int Return code (0 for success, 1 for error)
     */
    [[nodiscard]] static int handle_validate_all_command(const std::string& templates_path);
};

} // namespace cql
