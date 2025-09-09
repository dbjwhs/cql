// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "types.hpp"
#include "ailib/core/provider.hpp"
#include "ailib/providers/factory.hpp"
#include "ailib/core/config.hpp"
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>

/**
 * @file prompt_compiler.hpp
 * @brief LLM-powered meta-prompt compilation using AILib integration
 * 
 * This file provides the PromptCompiler class that uses the existing AILib
 * infrastructure to perform actual LLM-based meta-prompt optimization and
 * compilation through Claude API calls.
 */

namespace cql {
namespace meta_prompt {

/**
 * @brief Meta-prompt templates for different optimization strategies
 * 
 * These templates guide the LLM in performing specific types of 
 * meta-prompt compilation and optimization.
 */
struct MetaPromptTemplates {
    // Token reduction optimization
    static constexpr const char* TOKEN_OPTIMIZER = R"(
You are an expert prompt optimization specialist focused on token efficiency.

Your task is to optimize the following prompt to reduce token count while preserving semantic meaning and effectiveness.

OPTIMIZATION GUIDELINES:
- Remove redundant words and phrases
- Use more concise language where possible
- Eliminate unnecessary examples if the core concept is clear
- Maintain the prompt's intent, tone, and key instructions
- Preserve any specific formatting requirements
- Keep technical terms and domain-specific language intact

INPUT PROMPT:
{original_prompt}

DOMAIN CONTEXT: {domain}
OPTIMIZATION GOAL: Reduce tokens by {target_reduction}%

Please provide the optimized prompt that maintains semantic equivalence while using fewer tokens:)";

    // Accuracy improvement template
    static constexpr const char* ACCURACY_ENHANCER = R"(
You are an expert prompt engineering specialist focused on improving accuracy and clarity.

Your task is to enhance the following prompt to improve response accuracy and reduce ambiguity.

ENHANCEMENT GUIDELINES:
- Add specific instructions where ambiguity exists
- Include relevant examples if they would clarify expectations
- Specify output format requirements clearly
- Add constraints to prevent common misinterpretations
- Maintain conciseness while adding necessary detail
- Improve logical flow and structure

INPUT PROMPT:
{original_prompt}

DOMAIN CONTEXT: {domain}
TARGET USE CASE: {use_case}

Please provide the enhanced prompt with improved accuracy and clarity:)";

    // Domain-specific optimization
    static constexpr const char* DOMAIN_OPTIMIZER = R"(
You are an expert in {domain} prompt optimization with deep domain knowledge.

Your task is to optimize the following prompt specifically for {domain} applications.

DOMAIN OPTIMIZATION GUIDELINES:
- Apply domain-specific best practices and terminology
- Include relevant domain context and constraints
- Add domain-appropriate examples and use cases
- Ensure compliance with domain standards and conventions
- Optimize for typical {domain} workflows and outputs
- Leverage domain-specific prompt engineering techniques

INPUT PROMPT:
{original_prompt}

DOMAIN: {domain}
SPECIFIC REQUIREMENTS: {requirements}

Please provide the domain-optimized prompt:)";

    // Semantic validation template
    static constexpr const char* SEMANTIC_VALIDATOR = R"(
You are a prompt equivalence expert specializing in semantic analysis.

Your task is to analyze whether two prompts are semantically equivalent and will produce similar results.

ANALYSIS CRITERIA:
- Core intent and purpose alignment
- Key instruction preservation
- Output expectation consistency
- Constraint and requirement matching
- Context and domain appropriateness

ORIGINAL PROMPT:
{original_prompt}

OPTIMIZED PROMPT:
{optimized_prompt}

Please analyze semantic equivalence and provide your assessment as JSON:
{
    "is_semantically_equivalent": true/false,
    "confidence_score": 0.0-1.0,
    "key_differences": ["difference1", "difference2"],
    "risk_assessment": "low/medium/high",
    "recommendation": "accept/reject/modify"
})";
};

/**
 * @brief Configuration for LLM-based prompt compilation
 */
struct PromptCompilerConfig {
    std::string provider = "anthropic";           ///< AI provider to use
    std::string model = "claude-3-5-sonnet-20240620"; ///< Model for compilation
    std::string validation_model = "claude-3-haiku-20240307"; ///< Model for validation
    double temperature = 0.1;                     ///< Low temperature for consistency
    int max_tokens = 4096;                        ///< Maximum tokens per response
    std::chrono::seconds timeout{30};             ///< Request timeout
    bool enable_validation = true;                ///< Enable semantic validation
    double validation_confidence_threshold = 0.85; ///< Min confidence for auto-accept
};

/**
 * @brief LLM-powered prompt compiler using AILib integration
 * 
 * PromptCompiler provides actual LLM-based meta-prompt optimization
 * by integrating with the existing AILib infrastructure. It handles
 * template selection, API communication, response parsing, and result
 * validation through Claude API calls.
 */
class PromptCompiler {
public:
    /**
     * @brief Constructor with custom configuration
     * 
     * @param config PromptCompilerConfig with LLM settings
     * @param ailib_config AILib configuration for provider setup
     */
    explicit PromptCompiler(const PromptCompilerConfig& config = {},
                           std::shared_ptr<cql::Config> ailib_config = nullptr);

    /**
     * @brief Destructor
     */
    ~PromptCompiler();

    /**
     * @brief Compile meta-prompt using LLM optimization
     * 
     * Performs LLM-based meta-prompt compilation by selecting appropriate
     * templates, making API calls, and validating results for semantic
     * equivalence with the original prompt.
     * 
     * @param query Original prompt to optimize
     * @param flags Compilation flags specifying optimization strategy
     * @return CompilationResult with optimized prompt and metrics
     * @throws std::runtime_error on API errors or validation failures
     */
    [[nodiscard]] CompilationResult compile(std::string_view query,
                                           const CompilerFlags& flags);

    /**
     * @brief Asynchronous prompt compilation
     * 
     * @param query Original prompt to optimize  
     * @param flags Compilation flags
     * @return Future containing compilation result
     */
    [[nodiscard]] std::future<CompilationResult> compile_async(
        std::string_view query, const CompilerFlags& flags);

    /**
     * @brief Validate semantic equivalence of two prompts
     * 
     * Uses LLM-based semantic analysis to determine if an optimized
     * prompt maintains semantic equivalence with the original.
     * 
     * @param original Original prompt
     * @param optimized Optimized prompt to validate
     * @return ValidationResult with equivalence assessment
     */
    [[nodiscard]] ValidationResult validate_semantic_equivalence(
        std::string_view original, std::string_view optimized);

    /**
     * @brief Check if LLM compilation is available
     * 
     * Verifies that the AI provider is properly configured and accessible.
     * 
     * @return true if LLM compilation can be performed
     */
    [[nodiscard]] bool is_available() const;

    /**
     * @brief Get estimated cost for compilation
     * 
     * @param query Query to estimate cost for
     * @param flags Compilation flags (affects template selection)
     * @return Estimated cost in USD, or nullopt if unavailable
     */
    [[nodiscard]] std::optional<double> estimate_cost(
        std::string_view query, const CompilerFlags& flags) const;

    /**
     * @brief Update configuration
     * 
     * @param config New configuration settings
     */
    void update_config(const PromptCompilerConfig& config);

    /**
     * @brief Get current configuration
     * 
     * @return Current PromptCompilerConfig
     */
    [[nodiscard]] const PromptCompilerConfig& get_config() const;

private:
    /**
     * @brief Select appropriate meta-prompt template
     */
    [[nodiscard]] std::string select_template(const CompilerFlags& flags) const;

    /**
     * @brief Build prompt with template substitution
     */
    [[nodiscard]] std::string build_prompt(const std::string& template_str,
                                          std::string_view query,
                                          const CompilerFlags& flags) const;

    /**
     * @brief Parse LLM response and extract optimized prompt
     */
    [[nodiscard]] std::string parse_optimization_response(
        const ProviderResponse& response) const;

    /**
     * @brief Parse validation response to ValidationResult
     */
    [[nodiscard]] ValidationResult parse_validation_response(
        const ProviderResponse& response) const;

    /**
     * @brief Calculate compilation metrics
     */
    void update_compilation_metrics(CompilationResult& result,
                                   const ProviderResponse& response,
                                   std::chrono::steady_clock::time_point start_time) const;

    // Configuration and state
    PromptCompilerConfig m_config;
    std::shared_ptr<cql::Config> m_ailib_config;
    std::unique_ptr<cql::AIProvider> m_provider;
    std::unique_ptr<cql::AIProvider> m_validation_provider;

    // Template cache for performance
    mutable std::unordered_map<std::string, std::string> m_template_cache;
};

} // namespace meta_prompt
} // namespace cql
