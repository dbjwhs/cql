// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "compiler.hpp"
#include "types.hpp"
#include <memory>
#include <mutex>
#include <atomic>

/**
 * @file hybrid_compiler.hpp
 * @brief Concrete implementation of the HybridCompiler for meta-prompt compilation
 * 
 * This file provides the main implementation that orchestrates local compilation,
 * cached LLM optimization, and full LLM-powered meta-compilation.
 */

namespace cql {
namespace meta_prompt {

// Forward declarations
class LocalCompiler;
class PromptCompiler;
class IntelligentCache;
class ValidationFramework;
class CircuitBreaker;
class CostController;

/**
 * @brief Concrete implementation of the hybrid compilation system
 * 
 * HybridCompilerImpl manages the complete meta-prompt compilation pipeline,
 * intelligently choosing between local compilation, cached results, and
 * LLM-powered optimization based on configuration and system state.
 */
class HybridCompilerImpl : public HybridCompiler {
public:
    /**
     * @brief Constructor with dependency injection
     */
    HybridCompilerImpl(std::shared_ptr<LocalCompiler> local_compiler,
                      std::shared_ptr<PromptCompiler> prompt_compiler,
                      std::shared_ptr<IntelligentCache> cache,
                      std::shared_ptr<ValidationFramework> validator,
                      std::shared_ptr<CircuitBreaker> circuit_breaker,
                      std::shared_ptr<CostController> cost_controller);

    /**
     * @brief Default constructor for standard configuration
     */
    HybridCompilerImpl();

    /**
     * @brief Destructor
     */
    ~HybridCompilerImpl() override;

    // Main compilation interface
    [[nodiscard]] CompilationResult compile(std::string_view query, 
                                           const CompilerFlags& flags = {}) override;

    [[nodiscard]] std::future<CompilationResult> compile_async(
        std::string_view query, const CompilerFlags& flags = {}) override;

    [[nodiscard]] std::vector<CompilationResult> compile_batch(
        const std::vector<std::string>& queries,
        const CompilerFlags& flags = {}) override;

    // System status and statistics
    [[nodiscard]] bool is_llm_available() const override;
    [[nodiscard]] CacheStatistics get_cache_statistics() const override;
    [[nodiscard]] CostStatistics get_cost_statistics() const override;

    // Cache management
    void warm_cache(const std::vector<std::string>& common_queries,
                    const CompilerFlags& flags = {}) override;
    void clear_cache() override;

    // Configuration
    void set_daily_budget(double budget_dollars) override;
    void set_validation_enabled(bool enabled) override;

private:
    /**
     * @brief Execute local-only compilation
     */
    [[nodiscard]] CompilationResult compile_local(std::string_view query,
                                                  const CompilerFlags& flags);

    /**
     * @brief Check cache for existing optimization
     */
    [[nodiscard]] std::optional<CompilationResult> check_cache(
        std::string_view query, const CompilerFlags& flags);

    /**
     * @brief Execute LLM-powered compilation
     */
    [[nodiscard]] CompilationResult compile_llm(std::string_view query,
                                                const CompilerFlags& flags);

    /**
     * @brief Validate compilation result for semantic equivalence
     */
    [[nodiscard]] ValidationResult validate_result(std::string_view original,
                                                   std::string_view compiled);

    /**
     * @brief Calculate compilation metrics
     */
    void update_metrics(CompilationResult& result, 
                       std::chrono::steady_clock::time_point start_time);

    // Component dependencies
    std::shared_ptr<LocalCompiler> m_local_compiler;
    std::shared_ptr<PromptCompiler> m_prompt_compiler;
    std::shared_ptr<IntelligentCache> m_cache;
    std::shared_ptr<ValidationFramework> m_validator;
    std::shared_ptr<CircuitBreaker> m_circuit_breaker;
    std::shared_ptr<CostController> m_cost_controller;

    // Configuration state
    std::atomic<bool> m_validation_enabled{true};
    std::atomic<double> m_daily_budget{10.0};

    // Thread safety
    mutable std::mutex m_stats_mutex;
    CacheStatistics m_cache_stats;
    CostStatistics m_cost_stats;
};

/**
 * @brief Local compilation backend for non-LLM processing
 * 
 * Provides fast, deterministic compilation using traditional
 * query processing techniques without API calls.
 */
class LocalCompiler {
public:
    /**
     * @brief Compile query using local processing only
     * 
     * @param query The CQL query to compile
     * @param flags Compilation flags (optimization hints)
     * @return CompilationResult with locally optimized query
     */
    [[nodiscard]] virtual CompilationResult compile(std::string_view query,
                                                    const CompilerFlags& flags) = 0;

    /**
     * @brief Check if local compilation is available
     */
    [[nodiscard]] virtual bool is_available() const = 0;

    virtual ~LocalCompiler() = default;
};

/**
 * @brief Default implementation of LocalCompiler using existing CQL infrastructure
 */
class DefaultLocalCompiler : public LocalCompiler {
public:
    DefaultLocalCompiler();
    ~DefaultLocalCompiler() override;

    [[nodiscard]] CompilationResult compile(std::string_view query,
                                           const CompilerFlags& flags) override;

    [[nodiscard]] bool is_available() const override { return true; }

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace meta_prompt
} // namespace cql
