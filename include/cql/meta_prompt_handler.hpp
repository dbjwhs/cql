// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "cql/meta_prompt/types.hpp"
#include <string>

namespace cql {

/**
 * @class MetaPromptHandler
 * @brief Handles meta-prompt compilation command-line operations
 * 
 * This class provides the CLI interface for meta-prompt compilation,
 * handling optimization requests and displaying results to users.
 */
class MetaPromptHandler {
public:
    /**
     * @brief Handle the --optimize command with various options
     * @param argc Argument count
     * @param argv Argument array  
     * @return CQL_NO_ERROR on success, CQL_ERROR on failure
     */
    static int handle_optimize_command(int argc, char* argv[]);
    
    /**
     * @brief Parse compilation mode from string
     * @param mode_str String representation of compilation mode
     * @return CompilationMode enum value
     */
    static meta_prompt::CompilationMode parse_compilation_mode(const std::string& mode_str);
    
    /**
     * @brief Parse optimization goal from string
     * @param goal_str String representation of optimization goal
     * @return OptimizationGoal enum value
     */
    static meta_prompt::OptimizationGoal parse_optimization_goal(const std::string& goal_str);
    
    /**
     * @brief Display compilation results with metrics
     * @param result The compilation result to display
     * @param show_metrics Whether to show detailed metrics
     * @param show_validation Whether to show validation details
     */
    static void display_compilation_result(const meta_prompt::CompilationResult& result,
                                         bool show_metrics = false,
                                         bool show_validation = false);
    
private:
    /**
     * @brief Display usage information for optimize command
     */
    static void print_optimize_usage();
    
    /**
     * @brief Format metrics for display
     * @param metrics The compilation metrics to format
     * @return Formatted string representation
     */
    static std::string format_metrics(const meta_prompt::CompilationMetrics& metrics);
    
    /**
     * @brief Format validation results for display
     * @param validation The validation result to format
     * @return Formatted string representation
     */
    static std::string format_validation(const meta_prompt::ValidationResult& validation);
};

} // namespace cql
