// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/meta_prompt_handler.hpp"
#include "../../include/cql/command_line_handler.hpp"
#include "../../include/cql/meta_prompt/hybrid_compiler.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/project_utils.hpp"
#include "../../include/cql/input_validator.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace cql {

int MetaPromptHandler::handle_optimize_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Input file required for --optimize option\n";
        print_optimize_usage();
        return CQL_ERROR;
    }

    try {
        CommandLineHandler cmd_handler(argc, argv);
        
        // Get input file (first positional argument after --optimize)
        std::string input_file = argv[2];
        
        // Validate input file path for security and resolve securely
        // resolve_path_securely() already includes all validation from validate_file_path()
        std::string secure_input_path = InputValidator::resolve_path_securely(input_file);
        
        // Parse compilation options
        meta_prompt::CompilationMode mode = meta_prompt::CompilationMode::CACHED_LLM;
        meta_prompt::OptimizationGoal goal = meta_prompt::OptimizationGoal::BALANCED;
        std::string domain;
        bool show_metrics = false;
        bool show_validation = false;
        
        // Parse --mode option
        if (auto mode_value = cmd_handler.get_option_value("--mode")) {
            mode = parse_compilation_mode(*mode_value);
        }
        
        // Parse --goal option
        if (auto goal_value = cmd_handler.get_option_value("--goal")) {
            goal = parse_optimization_goal(*goal_value);
        }
        
        // Parse --domain option
        if (auto domain_value = cmd_handler.get_option_value("--domain")) {
            domain = *domain_value;
            
            // Validate domain parameter for security
            if (domain.size() > InputValidator::MAX_CATEGORY_NAME_LENGTH) {
                std::cerr << "Error: Domain name too long (max: " << InputValidator::MAX_CATEGORY_NAME_LENGTH << ")\n";
                return CQL_ERROR;
            }
            
            // Check for safe characters only
            if (!InputValidator::contains_only_safe_chars(domain, "a-zA-Z0-9_-")) {
                std::cerr << "Error: Domain contains invalid characters. Only alphanumeric, underscore, and hyphen allowed.\n";
                return CQL_ERROR;
            }
        }
        
        // Parse display options
        show_metrics = cmd_handler.has_option("--show-metrics");
        show_validation = cmd_handler.has_option("--show-validation");
        
        // Parse --timeout option (in seconds)
        std::chrono::seconds timeout_override{0};
        if (auto timeout_value = cmd_handler.get_option_value("--timeout")) {
            try {
                int timeout_seconds = std::stoi(*timeout_value);
                if (timeout_seconds < 1 || timeout_seconds > 600) { // 1 second to 10 minutes
                    std::cerr << "Error: Timeout must be between 1 and 600 seconds\n";
                    return CQL_ERROR;
                }
                timeout_override = std::chrono::seconds{timeout_seconds};
                Logger::getInstance().log(LogLevel::INFO, 
                    "Using custom timeout: ", timeout_seconds, " seconds");
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid timeout value '" << *timeout_value << "'\n";
                return CQL_ERROR;
            }
        }
        
        Logger::getInstance().log(LogLevel::INFO,
            "Starting meta-prompt compilation for: ", 
            InputValidator::sanitize_for_logging(secure_input_path));
        
        // Read and compile the input query using validated path
        std::string query_content;
        try {
            query_content = util::read_file(secure_input_path);
            
            // Validate query content size
            InputValidator::validate_query_length(query_content);
            
        } catch (const std::exception& e) {
            std::cerr << "Error reading input file: " << e.what() << std::endl;
            Logger::getInstance().log(LogLevel::ERROR,
                "File read error: ", e.what());
            return CQL_ERROR;
        }
        
        std::string compiled_query;
        try {
            compiled_query = QueryProcessor::compile(query_content);
        } catch (const std::exception& e) {
            std::cerr << "Error compiling query: " << e.what() << std::endl;
            Logger::getInstance().log(LogLevel::ERROR,
                "Query compilation error: ", e.what());
            return CQL_ERROR;
        }
        
        Logger::getInstance().log(LogLevel::INFO,
            "Compiled query, starting optimization with mode: ", static_cast<int>(mode),
            ", goal: ", static_cast<int>(goal));
        
        // Create compiler flags
        meta_prompt::CompilerFlags flags;
        flags.mode = mode;
        flags.goal = goal;
        flags.domain = domain;
        if (timeout_override.count() > 0) {
            flags.custom_timeout = timeout_override;
        }
        
        // Create hybrid compiler with error handling
        std::unique_ptr<meta_prompt::HybridCompiler> compiler;
        try {
            compiler = meta_prompt::HybridCompiler::create();
            if (!compiler) {
                std::cerr << "Error: Failed to create meta-prompt compiler\n";
                Logger::getInstance().log(LogLevel::ERROR,
                    "HybridCompiler creation failed");
                return CQL_ERROR;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error creating compiler: " << e.what() << std::endl;
            Logger::getInstance().log(LogLevel::ERROR,
                "HybridCompiler creation exception: ", e.what());
            return CQL_ERROR;
        }
        
        // Perform meta-prompt compilation with error handling
        meta_prompt::CompilationResult result;
        try {
            result = compiler->compile(compiled_query, flags);
        } catch (const std::exception& e) {
            std::cerr << "Error during compilation: " << e.what() << std::endl;
            Logger::getInstance().log(LogLevel::ERROR,
                "Meta-prompt compilation exception: ", e.what());
            return CQL_ERROR;
        }
        
        // Display results
        display_compilation_result(result, show_metrics, show_validation);
        
        // Handle output with security validation
        if (result.success) {
            std::string output_file;
            if (argc > 3 && argv[3][0] != '-') {
                output_file = argv[3];
                
                // Validate output file path
                try {
                    // resolve_path_securely() already includes all validation from validate_file_path()
                    std::string secure_output_path = InputValidator::resolve_path_securely(output_file);
                    
                    // Validate response size before writing
                    InputValidator::validate_response_size(result.compiled_prompt);
                    
                    util::write_file(secure_output_path, result.compiled_prompt);
                    std::cout << "\nOptimized query written to: " 
                              << InputValidator::sanitize_for_logging(secure_output_path) << std::endl;
                              
                } catch (const SecurityValidationError& e) {
                    std::cerr << "Security validation error for output file: " << e.what() << std::endl;
                    Logger::getInstance().log(LogLevel::ERROR,
                        "Output file security validation failed: ", e.what());
                    return CQL_ERROR;
                } catch (const std::exception& e) {
                    std::cerr << "Error writing output file: " << e.what() << std::endl;
                    Logger::getInstance().log(LogLevel::ERROR,
                        "Output file write error: ", e.what());
                    return CQL_ERROR;
                }
            } else {
                try {
                    // Validate response size before displaying
                    InputValidator::validate_response_size(result.compiled_prompt);
                    
                    std::cout << "\n--- OPTIMIZED QUERY ---\n";
                    std::cout << result.compiled_prompt << std::endl;
                } catch (const SecurityValidationError& e) {
                    std::cerr << "Response too large to display: " << e.what() << std::endl;
                    Logger::getInstance().log(LogLevel::ERROR,
                        "Response size validation failed: ", e.what());
                    return CQL_ERROR;
                }
            }
            return CQL_NO_ERROR;
        } else {
            std::cerr << "Meta-prompt compilation failed\n";
            return CQL_ERROR;
        }
        
    } catch (const SecurityValidationError& e) {
        std::cerr << "Security validation error: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR,
            "Security validation failed: ", e.what());
        return CQL_ERROR;
    } catch (const std::exception& e) {
        std::cerr << "Error during meta-prompt compilation: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR,
            "Meta-prompt compilation exception: ", e.what());
        return CQL_ERROR;
    }
}

meta_prompt::CompilationMode MetaPromptHandler::parse_compilation_mode(const std::string& mode_str) {
    if (mode_str == "LOCAL_ONLY") {
        return meta_prompt::CompilationMode::LOCAL_ONLY;
    } else if (mode_str == "CACHED_LLM") {
        return meta_prompt::CompilationMode::CACHED_LLM;
    } else if (mode_str == "FULL_LLM") {
        return meta_prompt::CompilationMode::FULL_LLM;
    } else if (mode_str == "ASYNC_LLM") {
        return meta_prompt::CompilationMode::ASYNC_LLM;
    } else {
        std::cerr << "Warning: Unknown compilation mode '" << mode_str 
                  << "', using CACHED_LLM as default\n";
        return meta_prompt::CompilationMode::CACHED_LLM;
    }
}

meta_prompt::OptimizationGoal MetaPromptHandler::parse_optimization_goal(const std::string& goal_str) {
    if (goal_str == "REDUCE_TOKENS") {
        return meta_prompt::OptimizationGoal::REDUCE_TOKENS;
    } else if (goal_str == "IMPROVE_ACCURACY") {
        return meta_prompt::OptimizationGoal::IMPROVE_ACCURACY;
    } else if (goal_str == "BALANCED") {
        return meta_prompt::OptimizationGoal::BALANCED;
    } else if (goal_str == "DOMAIN_SPECIFIC") {
        return meta_prompt::OptimizationGoal::DOMAIN_SPECIFIC;
    } else {
        std::cerr << "Warning: Unknown optimization goal '" << goal_str 
                  << "', using BALANCED as default\n";
        return meta_prompt::OptimizationGoal::BALANCED;
    }
}

void MetaPromptHandler::display_compilation_result(const meta_prompt::CompilationResult& result,
                                                  bool show_metrics,
                                                  bool show_validation) {
    std::cout << "\n=== META-PROMPT COMPILATION RESULTS ===" << std::endl;
    
    if (result.success) {
        std::cout << "Status: ✅ SUCCESS" << std::endl;
        
        // Basic info
        std::cout << "Original length: " << result.original_query.length() << " characters" << std::endl;
        std::cout << "Optimized length: " << result.compiled_prompt.length() << " characters" << std::endl;
        
        if (!result.original_query.empty()) {
            double reduction = ((double)(result.original_query.length() - result.compiled_prompt.length()) 
                              / result.original_query.length()) * 100.0;
            std::cout << "Size change: " << std::fixed << std::setprecision(1) << reduction << "%" << std::endl;
        }
        
        // Show metrics if requested
        if (show_metrics) {
            std::cout << "\n" << format_metrics(result.metrics) << std::endl;
        }
        
        // Show validation if requested  
        if (show_validation) {
            std::cout << "\n" << format_validation(result.validation_result) << std::endl;
        }
        
    } else {
        std::cout << "Status: ❌ FAILED" << std::endl;
        std::cout << "Error: " << result.error_message << std::endl;
        
        if (show_metrics) {
            std::cout << "\n" << format_metrics(result.metrics) << std::endl;
        }
    }
}

void MetaPromptHandler::print_optimize_usage() {
    std::cout << "Usage: cql --optimize INPUT_FILE [OUTPUT_FILE] [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --mode <mode>           Compilation mode (LOCAL_ONLY, CACHED_LLM, FULL_LLM, ASYNC_LLM)\n";
    std::cout << "  --goal <goal>           Optimization goal (REDUCE_TOKENS, IMPROVE_ACCURACY, BALANCED, DOMAIN_SPECIFIC)\n";
    std::cout << "  --domain <domain>       Domain context for optimization\n";
    std::cout << "  --timeout <seconds>     Network timeout in seconds (1-600, default: 120)\n";
    std::cout << "  --show-metrics          Display detailed compilation metrics\n";
    std::cout << "  --show-validation       Display semantic validation results\n\n";
    std::cout << "Examples:\n";
    std::cout << "  cql --optimize query.cql                    # Basic optimization\n";
    std::cout << "  cql --optimize query.cql --goal REDUCE_TOKENS --show-metrics\n";
    std::cout << "  cql --optimize query.cql optimized.cql --mode FULL_LLM --domain software --timeout 60\n";
}

std::string MetaPromptHandler::format_metrics(const meta_prompt::CompilationMetrics& metrics) {
    std::ostringstream oss;
    oss << "--- COMPILATION METRICS ---\n";
    oss << "Compilation time: " << metrics.compilation_time.count() << " ms\n";
    oss << "Cache hit: " << (metrics.cache_hit ? "Yes" : "No") << "\n";
    oss << "Used LLM: " << (metrics.used_llm ? "Yes" : "No") << "\n";
    
    if (metrics.used_llm) {
        oss << "Input tokens: " << metrics.input_tokens << "\n";
        oss << "Output tokens: " << metrics.output_tokens << "\n";
        oss << "LLM API time: " << metrics.llm_api_time.count() << " ms\n";
        
        if (metrics.token_reduction_percent > 0) {
            oss << "Token reduction: " << std::fixed << std::setprecision(1) 
                << metrics.token_reduction_percent << "%\n";
        }
        
        if (metrics.actual_cost > 0) {
            oss << "Actual cost: $" << std::fixed << std::setprecision(4) 
                << metrics.actual_cost << "\n";
        }
    }
    
    return oss.str();
}

std::string MetaPromptHandler::format_validation(const meta_prompt::ValidationResult& validation) {
    std::ostringstream oss;
    oss << "--- SEMANTIC VALIDATION ---\n";
    oss << "Semantically equivalent: " << (validation.is_semantically_equivalent ? "✅ Yes" : "❌ No") << "\n";
    oss << "Confidence score: " << std::fixed << std::setprecision(2) << validation.confidence_score << "\n";
    oss << "Validation method: " << validation.validation_method << "\n";
    
    return oss.str();
}

} // namespace cql
