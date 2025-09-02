// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "types.hpp"
#include <string_view>
#include <future>
#include <functional>

/**
 * @file compiler.hpp
 * @brief Main interface for the CQL Meta-Prompt Compiler System
 * 
 * This file defines the primary HybridCompiler class that orchestrates
 * the revolutionary meta-compilation process using LLM as a compilation backend.
 * 
 * The HybridCompiler provides a seamless interface that can operate in multiple
 * modes from pure local compilation to full AI-powered optimization.
 */

namespace cql {
namespace meta_prompt {

/**
 * @brief Main orchestrator for the hybrid compilation pipeline
 * 
 * The HybridCompiler is the primary entry point for meta-prompt compilation.
 * It manages the decision between local compilation, cached LLM optimization,
 * and full LLM-powered meta-compilation based on configuration and availability.
 * 
 * Key Features:
 * - Multiple compilation modes (LOCAL_ONLY, CACHED_LLM, FULL_LLM)
 * - Automatic fallback to local compilation on failures
 * - Comprehensive caching and cost management
 * - Real-time metrics and performance monitoring
 * - Semantic validation of optimized prompts
 * 
 * Example Usage:
 * @code
 * auto compiler = HybridCompiler::create();
 * 
 * CompilerFlags flags{
 *     .mode = CompilationMode::CACHED_LLM,
 *     .goal = OptimizationGoal::REDUCE_TOKENS,
 *     .domain = "system_programming",
 *     .cost_budget = 0.01
 * };
 * 
 * auto result = compiler->compile("CODEREQ cpp class Calculator { ... }", flags);
 * if (result.success) {
 *     std::cout << "Optimized prompt: " << result.compiled_prompt << std::endl;
 *     std::cout << "Token reduction: " << result.metrics.token_reduction_percent << "%" << std::endl;
 * }
 * @endcode
 */
class HybridCompiler {
public:
    /**
     * @brief Create a new HybridCompiler instance
     * 
     * Factory method for creating properly initialized HybridCompiler instances
     * with all necessary components (cache, validators, API clients, etc.)
     * 
     * @return std::unique_ptr<HybridCompiler> Ready-to-use compiler instance
     */
    [[nodiscard]] static std::unique_ptr<HybridCompiler> create();
    
    /**
     * @brief Create HybridCompiler with custom configuration
     * 
     * @param config Configuration options for components
     * @return std::unique_ptr<HybridCompiler> Configured compiler instance
     */
    [[nodiscard]] static std::unique_ptr<HybridCompiler> create_with_config(
        const std::unordered_map<std::string, std::string>& config);

    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~HybridCompiler() = default;

    /**
     * @brief Primary compilation interface
     * 
     * Compiles a CQL query using the specified mode and optimization goals.
     * Automatically handles caching, API failures, and fallback strategies.
     * 
     * @param query The CQL query to compile and optimize
     * @param flags Compilation configuration and optimization parameters
     * @return CompilationResult Complete result with optimized prompt and metrics
     */
    [[nodiscard]] virtual CompilationResult compile(std::string_view query, 
                                                   const CompilerFlags& flags = {}) = 0;

    /**
     * @brief Asynchronous compilation for non-blocking workflows
     * 
     * Performs compilation in a background thread, returning a future that
     * can be awaited or polled. Ideal for UI applications and batch processing.
     * 
     * @param query The CQL query to compile
     * @param flags Compilation configuration
     * @return std::future<CompilationResult> Future for the compilation result
     */
    [[nodiscard]] virtual std::future<CompilationResult> compile_async(
        std::string_view query, const CompilerFlags& flags = {}) = 0;

    /**
     * @brief Batch compilation for efficiency
     * 
     * Compiles multiple queries in a single operation, optimizing for
     * throughput and cost efficiency through batching and parallel processing.
     * 
     * @param queries Vector of queries to compile
     * @param flags Shared compilation configuration
     * @return std::vector<CompilationResult> Results for each query
     */
    [[nodiscard]] virtual std::vector<CompilationResult> compile_batch(
        const std::vector<std::string>& queries,
        const CompilerFlags& flags = {}) = 0;

    /**
     * @brief Check if LLM compilation is currently available
     * 
     * Verifies API connectivity, rate limits, circuit breaker state,
     * and other factors that might affect LLM-powered compilation.
     * 
     * @return bool True if LLM compilation is available
     */
    [[nodiscard]] virtual bool is_llm_available() const = 0;

    /**
     * @brief Get current cache statistics
     * 
     * @return CacheStatistics Current cache performance metrics
     */
    [[nodiscard]] virtual CacheStatistics get_cache_statistics() const = 0;

    /**
     * @brief Get current cost statistics
     * 
     * @return CostStatistics Current cost tracking and budget information
     */
    [[nodiscard]] virtual CostStatistics get_cost_statistics() const = 0;

    /**
     * @brief Warm up the cache with common queries
     * 
     * Pre-loads the cache with optimized versions of commonly used queries
     * to improve performance during normal operations.
     * 
     * @param common_queries List of queries to pre-optimize
     * @param flags Default flags for pre-optimization
     */
    virtual void warm_cache(const std::vector<std::string>& common_queries,
                          const CompilerFlags& flags = {}) = 0;

    /**
     * @brief Clear all cached optimizations
     * 
     * Removes all cached entries, forcing fresh optimization on next compilation.
     * Useful for testing or when optimization strategies have changed.
     */
    virtual void clear_cache() = 0;

    /**
     * @brief Set daily cost budget
     * 
     * Updates the daily spending limit for LLM API calls. When exceeded,
     * the compiler will fall back to local-only compilation.
     * 
     * @param budget_dollars Daily budget in USD
     */
    virtual void set_daily_budget(double budget_dollars) = 0;

    /**
     * @brief Enable or disable semantic validation
     * 
     * Controls whether optimized prompts are validated for semantic
     * equivalence with the original query.
     * 
     * @param enabled True to enable validation, false to disable
     */
    virtual void set_validation_enabled(bool enabled) = 0;

protected:
    /**
     * @brief Protected constructor for inheritance
     * 
     * HybridCompiler instances should be created through the factory methods
     * to ensure proper initialization of all components.
     */
    HybridCompiler() = default;

    /**
     * @brief Disable copy construction
     */
    HybridCompiler(const HybridCompiler&) = delete;

    /**
     * @brief Disable copy assignment
     */
    HybridCompiler& operator=(const HybridCompiler&) = delete;
};

/**
 * @brief Callback function type for compilation progress updates
 * 
 * Used for monitoring long-running compilation operations, particularly
 * useful for batch processing and async operations.
 * 
 * Parameters: current_item, total_items, current_result
 */
using CompilationProgressCallback = std::function<void(size_t, size_t, const CompilationResult&)>;

/**
 * @brief Callback function type for cost threshold warnings
 * 
 * Invoked when compilation costs approach configured budgets,
 * allowing applications to take appropriate action.
 * 
 * Parameters: current_cost, budget_limit, utilization_percent
 */
using CostWarningCallback = std::function<void(double, double, double)>;

/**
 * @brief Global configuration for meta-prompt compiler system
 * 
 * Provides system-wide settings that affect all HybridCompiler instances.
 * These settings are typically loaded from environment variables or config files.
 */
struct GlobalCompilerConfig {
    std::string anthropic_api_key;
    std::string default_model = "claude-haiku-3.5";
    double default_daily_budget = 10.0;
    size_t max_cache_size_mb = 100;
    std::chrono::seconds cache_ttl{3600}; // 1 hour
    bool enable_metrics_collection = true;
    CompilationProgressCallback progress_callback;
    CostWarningCallback cost_warning_callback;
};

/**
 * @brief Initialize the meta-prompt compiler system
 * 
 * Must be called once before creating any HybridCompiler instances.
 * Initializes global resources, logging, and configuration.
 * 
 * @param config Global configuration settings
 * @return bool True if initialization succeeded
 */
[[nodiscard]] bool initialize_compiler_system(const GlobalCompilerConfig& config = {});

/**
 * @brief Shutdown the meta-prompt compiler system
 * 
 * Cleans up global resources and ensures all background tasks complete.
 * Should be called before application termination.
 */
void shutdown_compiler_system();

/**
 * @brief Get the current global configuration
 * 
 * @return const GlobalCompilerConfig& Current configuration
 */
[[nodiscard]] const GlobalCompilerConfig& get_global_config();

} // namespace meta_prompt
} // namespace cql
