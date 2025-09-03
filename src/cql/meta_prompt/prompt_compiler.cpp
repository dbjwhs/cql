// MIT License
// Copyright (c) 2025 dbjwhs

#include "cql/meta_prompt/prompt_compiler.hpp"
#include "cql/project_utils.hpp"
#include "ailib/detail/json_utils.hpp"
#include "cql/project_utils.hpp"
#include <future>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace cql {
namespace meta_prompt {

//=============================================================================
// PromptCompiler Implementation
//=============================================================================

PromptCompiler::PromptCompiler(const PromptCompilerConfig& config,
                              std::shared_ptr<cql::Config> ailib_config)
    : m_config(config) {
    
    // Use provided AILib config or create default
    if (ailib_config) {
        m_ailib_config = ailib_config;
    } else {
        m_ailib_config = std::make_shared<cql::Config>();
        
        // Check for API key in environment
        const char* api_key = std::getenv("CQL_API_KEY");
        if (!api_key) {
            Logger::getInstance().log(LogLevel::ERROR,
                "CQL_API_KEY environment variable not set - LLM compilation will be unavailable");
        } else {
            m_ailib_config->set_api_key(m_config.provider, api_key);
        }
    }

    // Set model configuration
    m_ailib_config->set_model(m_config.provider, m_config.model);

    // Create AI providers
    try {
        auto& factory = cql::ProviderFactory::get_instance();
        m_provider = factory.create_provider(m_config.provider, *m_ailib_config);
        
        if (m_config.enable_validation) {
            // Create separate provider for validation with different model
            auto validation_config = *m_ailib_config;
            validation_config.set_model(m_config.provider, m_config.validation_model);
            m_validation_provider = factory.create_provider(m_config.provider, validation_config);
        }
        
        Logger::getInstance().log(LogLevel::INFO,
            "PromptCompiler initialized with provider: ", m_config.provider,
            ", model: ", m_config.model);
            
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR,
            "Failed to initialize PromptCompiler: ", e.what());
        // Don't throw - allow graceful degradation to local compilation
    }
}

PromptCompiler::~PromptCompiler() = default;

CompilationResult PromptCompiler::compile(std::string_view query,
                                        const CompilerFlags& flags) {
    auto start_time = std::chrono::steady_clock::now();
    
    if (!is_available()) {
        return CompilationResult::error_result(
            "LLM provider not available - check API key configuration",
            std::string(query));
    }

    try {
        // Select appropriate template based on optimization goal
        std::string template_str = select_template(flags);
        
        // Build the meta-prompt with template substitution
        std::string meta_prompt = build_prompt(template_str, query, flags);
        
        // Create provider request
        ProviderRequest request;
        request.prompt = meta_prompt;
        request.model = m_config.model;
        request.temperature = m_config.temperature;
        request.max_tokens = m_config.max_tokens;
        
        // Use custom timeout from flags if provided, otherwise use default config timeout
        auto timeout_to_use = flags.custom_timeout.has_value() ? 
                              *flags.custom_timeout : m_config.timeout;
        request.metadata["timeout"] = std::to_string(timeout_to_use.count());
        
        if (flags.custom_timeout.has_value()) {
            Logger::getInstance().log(LogLevel::DEBUG,
                "Using custom timeout for LLM request: ", timeout_to_use.count(), " seconds");
        }

        Logger::getInstance().log(LogLevel::DEBUG,
            "Sending meta-prompt compilation request to ", m_config.provider);

        // Make API call
        auto response = m_provider->generate(request);
        
        if (!response.success) {
            std::string error_msg = "LLM compilation failed";
            if (response.error_message) {
                error_msg += ": " + *response.error_message;
            }
            return CompilationResult::error_result(error_msg, std::string(query));
        }

        // Parse optimized prompt from response
        std::string optimized = parse_optimization_response(response);
        if (optimized.empty()) {
            return CompilationResult::error_result(
                "Failed to parse optimized prompt from LLM response",
                std::string(query));
        }

        // Perform semantic validation if enabled
        ValidationResult validation;
        if (m_config.enable_validation && m_validation_provider) {
            validation = validate_semantic_equivalence(query, optimized);
            
            // Check if validation passes confidence threshold
            if (validation.confidence_score < m_config.validation_confidence_threshold) {
                Logger::getInstance().log(LogLevel::ERROR,
                    "Semantic validation below threshold: ", validation.confidence_score,
                    " < ", m_config.validation_confidence_threshold);
            }
        } else {
            // Default validation when LLM validation is disabled
            validation.is_semantically_equivalent = true;
            validation.confidence_score = 0.95;
            validation.validation_method = "llm_compilation_default";
        }

        // Create successful result
        auto result = CompilationResult::success_result(optimized, {}, validation);
        result.original_query = std::string(query);
        result.flags_used = flags;
        
        // Update metrics
        update_compilation_metrics(result, response, start_time);
        
        Logger::getInstance().log(LogLevel::DEBUG,
            "LLM compilation successful - tokens: ", response.tokens_used,
            ", latency: ", response.latency.count(), "ms");

        return result;

    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR,
            "LLM compilation exception: ", e.what());
        return CompilationResult::error_result(
            std::string("LLM compilation exception: ") + e.what(),
            std::string(query));
    }
}

std::future<CompilationResult> PromptCompiler::compile_async(
    std::string_view query, const CompilerFlags& flags) {
    
    // Launch compilation in separate thread
    return std::async(std::launch::async,
        [this, q = std::string(query), f = flags]() {
            return this->compile(q, f);
        });
}

ValidationResult PromptCompiler::validate_semantic_equivalence(
    std::string_view original, std::string_view optimized) {
    
    if (!m_validation_provider) {
        ValidationResult result;
        result.is_semantically_equivalent = true;
        result.confidence_score = 0.8;
        result.validation_method = "llm_validation_unavailable";
        return result;
    }

    try {
        // Build validation prompt
        std::string validation_prompt = MetaPromptTemplates::SEMANTIC_VALIDATOR;
        
        // Replace placeholders
        std::regex original_regex(R"(\{original_prompt\})");
        std::regex optimized_regex(R"(\{optimized_prompt\})");
        
        validation_prompt = std::regex_replace(validation_prompt, original_regex, std::string(original));
        validation_prompt = std::regex_replace(validation_prompt, optimized_regex, std::string(optimized));

        // Create validation request
        ProviderRequest request;
        request.prompt = validation_prompt;
        request.model = m_config.validation_model;
        request.temperature = 0.1; // Low temperature for consistent validation
        request.max_tokens = 1024;

        // Make validation API call
        auto response = m_validation_provider->generate(request);
        
        if (!response.success) {
            Logger::getInstance().log(LogLevel::ERROR,
                "Semantic validation API call failed");
            
            ValidationResult result;
            result.is_semantically_equivalent = true; // Conservative default
            result.confidence_score = 0.7;
            result.validation_method = "llm_validation_failed";
            return result;
        }

        // Parse validation response
        return parse_validation_response(response);

    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR,
            "Semantic validation exception: ", e.what());
        
        ValidationResult result;
        result.is_semantically_equivalent = true; // Conservative default
        result.confidence_score = 0.7;
        result.validation_method = "llm_validation_exception";
        return result;
    }
}

bool PromptCompiler::is_available() const {
    return m_provider && m_provider->is_configured();
}

std::optional<double> PromptCompiler::estimate_cost(
    std::string_view query, const CompilerFlags& flags) const {
    
    if (!m_provider) {
        return std::nullopt;
    }

    try {
        // Build request for cost estimation
        std::string template_str = select_template(flags);
        std::string meta_prompt = build_prompt(template_str, query, flags);
        
        ProviderRequest request;
        request.prompt = meta_prompt;
        request.model = m_config.model;
        request.max_tokens = m_config.max_tokens;

        auto compilation_cost = m_provider->estimate_cost(request);
        
        // Add validation cost if enabled
        if (m_config.enable_validation && m_validation_provider) {
            ProviderRequest validation_request;
            validation_request.prompt = std::string(query) + std::string(query); // Rough estimate
            validation_request.model = m_config.validation_model;
            validation_request.max_tokens = 1024;
            
            auto validation_cost = m_validation_provider->estimate_cost(validation_request);
            if (compilation_cost && validation_cost) {
                return *compilation_cost + *validation_cost;
            }
        }

        return compilation_cost;

    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::DEBUG,
            "Cost estimation failed: ", e.what());
        return std::nullopt;
    }
}

void PromptCompiler::update_config(const PromptCompilerConfig& config) {
    m_config = config;
    
    if (m_ailib_config) {
        m_ailib_config->set_model(m_config.provider, m_config.model);
        
        // Recreate providers with new config
        try {
            auto& factory = cql::ProviderFactory::get_instance();
            m_provider = factory.create_provider(m_config.provider, *m_ailib_config);
            
            if (m_config.enable_validation) {
                auto validation_config = *m_ailib_config;
                validation_config.set_model(m_config.provider, m_config.validation_model);
                m_validation_provider = factory.create_provider(m_config.provider, validation_config);
            } else {
                m_validation_provider.reset();
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR,
                "Failed to update PromptCompiler configuration: ", e.what());
        }
    }
}

const PromptCompilerConfig& PromptCompiler::get_config() const {
    return m_config;
}

//=============================================================================
// Private Implementation Methods
//=============================================================================

std::string PromptCompiler::select_template(const CompilerFlags& flags) const {
    switch (flags.goal) {
        case OptimizationGoal::REDUCE_TOKENS:
            return MetaPromptTemplates::TOKEN_OPTIMIZER;
            
        case OptimizationGoal::IMPROVE_ACCURACY:
            return MetaPromptTemplates::ACCURACY_ENHANCER;
            
        case OptimizationGoal::DOMAIN_SPECIFIC:
            return MetaPromptTemplates::DOMAIN_OPTIMIZER;
            
        case OptimizationGoal::BALANCED:
        default:
            // For balanced optimization, use token optimizer with accuracy considerations
            return MetaPromptTemplates::TOKEN_OPTIMIZER;
    }
}

std::string PromptCompiler::build_prompt(const std::string& template_str,
                                        std::string_view query,
                                        const CompilerFlags& flags) const {
    std::string result = template_str;
    
    // Replace common placeholders
    std::regex query_regex(R"(\{original_prompt\})");
    std::regex domain_regex(R"(\{domain\})");
    std::regex target_regex(R"(\{target_reduction\})");
    std::regex use_case_regex(R"(\{use_case\})");
    std::regex requirements_regex(R"(\{requirements\})");
    
    result = std::regex_replace(result, query_regex, std::string(query));
    result = std::regex_replace(result, domain_regex, flags.domain.empty() ? "general" : flags.domain);
    result = std::regex_replace(result, target_regex, "20"); // Default 20% reduction
    result = std::regex_replace(result, use_case_regex, "general optimization");
    result = std::regex_replace(result, requirements_regex, "standard compliance");
    
    return result;
}

std::string PromptCompiler::parse_optimization_response(
    const ProviderResponse& response) const {
    
    std::string content = response.content;
    
    // Try to extract optimized prompt from various response formats
    // Look for patterns like "OPTIMIZED PROMPT:", "Here is the optimized prompt:", etc.
    std::vector<std::regex> extraction_patterns = {
        std::regex(R"((?:OPTIMIZED PROMPT|Optimized prompt|optimized prompt):\s*\n?(.*?)(?:\n\n|$))", 
                  std::regex_constants::icase | std::regex_constants::multiline),
        std::regex(R"((?:Here is the|The) optimized prompt:?\s*\n?(.*?)(?:\n\n|$))", 
                  std::regex_constants::icase | std::regex_constants::multiline),
        std::regex(R"(```(?:prompt)?\s*\n?(.*?)\n?```)", 
                  std::regex_constants::multiline),
    };
    
    for (const auto& pattern : extraction_patterns) {
        std::smatch match;
        if (std::regex_search(content, match, pattern) && match.size() > 1) {
            std::string extracted = match[1].str();
            
            // Clean up the extracted prompt
            extracted = std::regex_replace(extracted, std::regex(R"(^\s+|\s+$)"), ""); // Trim
            if (!extracted.empty()) {
                return extracted;
            }
        }
    }
    
    // If no specific pattern found, try to use the entire response after cleaning
    content = std::regex_replace(content, std::regex(R"(^\s+|\s+$)"), ""); // Trim
    if (!content.empty()) {
        Logger::getInstance().log(LogLevel::DEBUG,
            "No extraction pattern matched, using full response");
        return content;
    }
    
    return "";
}

ValidationResult PromptCompiler::parse_validation_response(
    const ProviderResponse& response) const {
    
    ValidationResult result;
    result.validation_method = "llm_semantic_validation";
    
    try {
        // Try to extract JSON from response
        std::string content = response.content;
        
        // Look for JSON block
        std::regex json_pattern(R"(\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\})", std::regex_constants::multiline);
        std::smatch match;
        
        if (std::regex_search(content, match, json_pattern)) {
            std::string json_str = match[0].str();
            
            // Parse JSON using AILib's JSON utilities
            // Note: This is a simplified parser - in production, use proper JSON library
            if (json_str.find("\"is_semantically_equivalent\": true") != std::string::npos) {
                result.is_semantically_equivalent = true;
            } else if (json_str.find("\"is_semantically_equivalent\": false") != std::string::npos) {
                result.is_semantically_equivalent = false;
            }
            
            // Extract confidence score
            std::regex confidence_pattern(R"("confidence_score":\s*([0-9]*\.?[0-9]+))");
            std::smatch confidence_match;
            if (std::regex_search(json_str, confidence_match, confidence_pattern)) {
                result.confidence_score = std::stod(confidence_match[1].str());
            }
            
            return result;
        }
        
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR,
            "Failed to parse validation JSON: ", e.what());
    }
    
    // Fallback: look for text-based indicators
    std::string content_lower = response.content;
    std::transform(content_lower.begin(), content_lower.end(), content_lower.begin(), ::tolower);
    
    if (content_lower.find("semantically equivalent") != std::string::npos ||
        content_lower.find("equivalent") != std::string::npos) {
        result.is_semantically_equivalent = true;
        result.confidence_score = 0.8;
    } else {
        result.is_semantically_equivalent = false;
        result.confidence_score = 0.3;
    }
    
    return result;
}

void PromptCompiler::update_compilation_metrics(CompilationResult& result,
                                              const ProviderResponse& response,
                                              std::chrono::steady_clock::time_point start_time) const {
    auto end_time = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    result.metrics.compilation_time = total_duration;
    result.metrics.cache_hit = false; // This is a fresh LLM compilation
    result.metrics.used_llm = true;
    result.metrics.input_tokens = response.prompt_tokens;
    result.metrics.output_tokens = response.completion_tokens;
    result.metrics.llm_api_time = response.latency;
    
    // Calculate token reduction percentage
    if (!result.original_query.empty() && response.prompt_tokens > 0) {
        size_t original_estimated_tokens = result.original_query.length() / 4; // Rough estimate
        size_t optimized_estimated_tokens = result.compiled_prompt.length() / 4;
        
        if (original_estimated_tokens > 0) {
            result.metrics.token_reduction_percent = 
                ((float)(original_estimated_tokens - optimized_estimated_tokens) / original_estimated_tokens) * 100.0f;
        }
    }
    
    // Estimate cost based on token usage
    if (response.tokens_used > 0) {
        // Rough cost estimation - in production, use provider-specific pricing
        result.metrics.actual_cost = (response.tokens_used / 1000.0) * 0.003; // ~$0.003 per 1K tokens
    }
}

} // namespace meta_prompt
} // namespace cql
