// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <optional>
#include <vector>
#include <unordered_map>

/**
 * @file types.hpp
 * @brief Core types and forward declarations for the CQL Meta-Prompt Compiler System
 * 
 * This file provides the fundamental types and forward declarations used throughout
 * the Meta-Prompt Compiler system. This revolutionary system treats the Anthropic
 * Claude API as a compilation backend for prompt optimization.
 */

namespace cql {
namespace meta_prompt {

// Forward declarations for all major components
class HybridCompiler;
class PromptCompiler;
class IntelligentCache;
class ValidationFramework;
class CircuitBreaker;
class CostController;
class OptimizationEngine;

/**
 * @brief Compilation modes for the hybrid compiler system
 * 
 * Defines the different approaches to compilation, from pure local processing
 * to full LLM-powered optimization with various caching strategies.
 */
enum class CompilationMode {
    LOCAL_ONLY,     ///< Development: < 10ms, no API calls
    CACHED_LLM,     ///< Staging: < 50ms, cached optimizations preferred
    ASYNC_LLM,      ///< Non-blocking: optimization happens in background
    FULL_LLM        ///< Production: 300-500ms, full AI-powered optimization
};

/**
 * @brief Optimization goals for meta-compilation
 * 
 * Specifies the primary objective for prompt optimization, allowing the
 * system to choose appropriate strategies and trade-offs.
 */
enum class OptimizationGoal {
    REDUCE_TOKENS,    ///< Minimize token usage while preserving functionality
    IMPROVE_ACCURACY, ///< Enhance response quality and precision
    DOMAIN_SPECIFIC,  ///< Adapt for specific domain requirements
    BALANCED          ///< Optimize for overall effectiveness
};

/**
 * @brief Configuration flags for compilation behavior
 * 
 * Comprehensive configuration structure that controls all aspects of
 * meta-compilation including performance, cost, and quality parameters.
 */
struct CompilerFlags {
    CompilationMode mode = CompilationMode::LOCAL_ONLY;
    OptimizationGoal goal = OptimizationGoal::BALANCED;
    bool validate_semantics = true;
    bool enable_caching = true;
    bool use_deterministic = false;
    std::string domain = "general";
    double cost_budget = 0.01; // $0.01 per compilation
    float temperature = 0.1f;  // Low for deterministic compilation
};

/**
 * @brief Metrics collected during compilation process
 * 
 * Comprehensive metrics tracking performance, cost, and quality
 * indicators for the meta-compilation process.
 */
struct CompilationMetrics {
    std::chrono::milliseconds compilation_time{0};
    std::chrono::milliseconds llm_api_time{0};
    double estimated_cost = 0.0;
    double actual_cost = 0.0;
    bool cache_hit = false;
    bool used_llm = false;
    size_t input_tokens = 0;
    size_t output_tokens = 0;
    float token_reduction_percent = 0.0f;
};

/**
 * @brief Result of semantic validation process
 * 
 * Provides detailed information about whether an optimized prompt
 * maintains semantic equivalence with the original.
 */
struct ValidationResult {
    bool is_semantically_equivalent = false;
    double confidence_score = 0.0;
    std::vector<std::string> detected_issues;
    std::string validation_method;
};

/**
 * @brief Complete result of a compilation operation
 * 
 * Contains the optimized prompt, metrics, validation results,
 * and all metadata from the compilation process.
 */
struct CompilationResult {
    bool success = false;
    std::string compiled_prompt;
    std::string error_message;
    CompilationMetrics metrics;
    ValidationResult validation_result;
    std::string original_query;
    CompilerFlags flags_used;
    
    /**
     * @brief Create a successful compilation result
     */
    static CompilationResult success_result(const std::string& prompt, 
                                          const CompilationMetrics& metrics,
                                          const ValidationResult& validation = {}) {
        CompilationResult result;
        result.success = true;
        result.compiled_prompt = prompt;
        result.metrics = metrics;
        result.validation_result = validation;
        return result;
    }
    
    /**
     * @brief Create a failed compilation result
     */
    static CompilationResult error_result(const std::string& error,
                                        const std::string& original_query = "") {
        CompilationResult result;
        result.success = false;
        result.error_message = error;
        result.original_query = original_query;
        return result;
    }
};

/**
 * @brief Cache statistics and performance metrics
 * 
 * Provides insight into cache behavior and effectiveness
 * for monitoring and optimization purposes.
 */
struct CacheStatistics {
    size_t total_requests = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    double hit_rate = 0.0;
    size_t cache_size_bytes = 0;
    size_t entry_count = 0;
    std::chrono::system_clock::time_point last_cleanup;
};

/**
 * @brief Cost tracking and budget management data
 * 
 * Maintains financial metrics for API usage and budget compliance.
 */
struct CostStatistics {
    double daily_cost = 0.0;
    int daily_requests = 0;
    double average_cost = 0.0;
    double budget_utilization = 0.0;
    std::chrono::system_clock::time_point day_start;
};

} // namespace meta_prompt
} // namespace cql
