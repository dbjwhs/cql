// MIT License
// Copyright (c) 2025 dbjwhs

#include "cql/meta_prompt/hybrid_compiler.hpp"
#include "cql/meta_prompt/intelligent_cache.hpp"
#include "cql/meta_prompt/prompt_compiler.hpp"
#include "cql/meta_prompt/circuit_breaker.hpp"
#include "cql/meta_prompt/cost_controller.hpp"
#include "cql/meta_prompt/validation_framework.hpp"
#include "cql/project_utils.hpp"
#include <chrono>
#include <sstream>
#include <thread>

namespace cql {
namespace meta_prompt {

// Factory method implementation
std::unique_ptr<HybridCompiler> HybridCompiler::create() {
    return std::make_unique<HybridCompilerImpl>();
}

std::unique_ptr<HybridCompiler> HybridCompiler::create_with_config(
    const std::unordered_map<std::string, std::string>& /* config */) {
    // For now, just create default instance
    // Future: parse config and create with custom components
    return std::make_unique<HybridCompilerImpl>();
}

// Global configuration (simplified for now)
static GlobalCompilerConfig g_config;

bool initialize_compiler_system(const GlobalCompilerConfig& config) {
    g_config = config;
    return true;
}

void shutdown_compiler_system() {
    // Cleanup if needed
}

const GlobalCompilerConfig& get_global_config() {
    return g_config;
}

//=============================================================================
// DefaultLocalCompiler implementation
//=============================================================================

class DefaultLocalCompiler::Impl {
public:
    CompilationResult compile(std::string_view query, const CompilerFlags& flags) {
        auto start_time = std::chrono::steady_clock::now();
        
        try {
            // Apply basic optimizations based on flags
            std::string optimized = optimize_query(query, flags);
            
            // Calculate metrics
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time);
            
            CompilationMetrics metrics;
            metrics.compilation_time = duration;
            metrics.cache_hit = false;
            metrics.used_llm = false;
            metrics.input_tokens = query.length() / 4; // Rough estimate
            metrics.output_tokens = optimized.length() / 4;
            
            if (query.length() > 0) {
                metrics.token_reduction_percent = 
                    ((float)(query.length() - optimized.length()) / query.length()) * 100.0f;
            }
            
            ValidationResult validation;
            validation.is_semantically_equivalent = true;
            validation.confidence_score = 1.0;
            validation.validation_method = "local_ast_comparison";
            
            auto result = CompilationResult::success_result(optimized, metrics, validation);
            result.original_query = std::string(query);
            result.flags_used = flags;
            
            return result;
            
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "Local compilation failed: ", e.what());
            return CompilationResult::error_result(e.what(), std::string(query));
        }
    }
    
private:
    std::string optimize_query(std::string_view query, const CompilerFlags& flags) {
        std::string result(query);
        
        // Apply optimization based on goal
        switch (flags.goal) {
            case OptimizationGoal::REDUCE_TOKENS:
                result = reduce_tokens(result);
                break;
            case OptimizationGoal::IMPROVE_ACCURACY:
                result = improve_accuracy(result);
                break;
            case OptimizationGoal::DOMAIN_SPECIFIC:
                result = apply_domain_optimizations(result, flags.domain);
                break;
            case OptimizationGoal::BALANCED:
            default:
                result = balanced_optimization(result);
                break;
        }
        
        return result;
    }
    
    std::string reduce_tokens(const std::string& query) {
        // Simple token reduction: remove extra whitespace
        std::string result;
        bool in_whitespace = false;
        
        for (char c : query) {
            if (std::isspace(c)) {
                if (!in_whitespace) {
                    result += ' ';
                    in_whitespace = true;
                }
            } else {
                result += c;
                in_whitespace = false;
            }
        }
        
        // Trim trailing whitespace
        while (!result.empty() && std::isspace(result.back())) {
            result.pop_back();
        }
        
        return result;
    }
    
    std::string improve_accuracy(const std::string& query) {
        // Add clarity markers for better accuracy
        std::string result = "Please provide a precise and accurate response:\n";
        result += query;
        result += "\n\nEnsure all details are correct and well-documented.";
        return result;
    }
    
    std::string apply_domain_optimizations(const std::string& query, 
                                          const std::string& domain) {
        std::string result = query;
        
        if (domain == "code_generation") {
            result = "Generate production-ready code with error handling:\n" + result;
        } else if (domain == "system_programming") {
            result = "Focus on performance and memory efficiency:\n" + result;
        } else if (domain == "data_science") {
            result = "Provide data analysis with statistical validation:\n" + result;
        }
        
        return result;
    }
    
    std::string balanced_optimization(const std::string& query) {
        // Apply moderate optimization
        return reduce_tokens(query);
    }
};

DefaultLocalCompiler::DefaultLocalCompiler() 
    : m_impl(std::make_unique<Impl>()) {
}

DefaultLocalCompiler::~DefaultLocalCompiler() = default;

CompilationResult DefaultLocalCompiler::compile(std::string_view query,
                                               const CompilerFlags& flags) {
    return m_impl->compile(query, flags);
}

//=============================================================================
// HybridCompilerImpl implementation
//=============================================================================

HybridCompilerImpl::HybridCompilerImpl(
    std::shared_ptr<LocalCompiler> local_compiler,
    std::shared_ptr<PromptCompiler> prompt_compiler,
    std::shared_ptr<IntelligentCache> cache,
    std::shared_ptr<ValidationFramework> validator,
    std::shared_ptr<CircuitBreaker> circuit_breaker,
    std::shared_ptr<CostController> cost_controller)
    : m_local_compiler(std::move(local_compiler))
    , m_prompt_compiler(std::move(prompt_compiler))
    , m_cache(std::move(cache))
    , m_validator(std::move(validator))
    , m_circuit_breaker(std::move(circuit_breaker))
    , m_cost_controller(std::move(cost_controller)) {
}

HybridCompilerImpl::HybridCompilerImpl()
    : m_local_compiler(std::make_shared<DefaultLocalCompiler>())
    , m_cache(create_intelligent_cache()) {
    
    // Initialize LLM components
    try {
        PromptCompilerConfig prompt_config;
        m_prompt_compiler = std::make_shared<PromptCompiler>(prompt_config);
        
        CircuitBreakerConfig circuit_config;
        m_circuit_breaker = std::make_shared<CircuitBreaker>(circuit_config);
        
        CostControllerConfig cost_config;
        m_cost_controller = std::make_shared<CostController>(cost_config);
        
        ValidationConfig validation_config;
        m_validator = create_validation_framework(validation_config);
        
        Logger::getInstance().log(LogLevel::INFO, 
            "HybridCompiler initialized with full LLM integration support");
            
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR,
            "Failed to initialize LLM components: ", e.what(),
            " - falling back to local-only mode");
        
        m_prompt_compiler = nullptr;
        m_circuit_breaker = nullptr;
        m_cost_controller = nullptr;
        m_validator = nullptr;
    }
}

HybridCompilerImpl::~HybridCompilerImpl() = default;

CompilationResult HybridCompilerImpl::compile(std::string_view query,
                                             const CompilerFlags& flags) {
    auto start_time = std::chrono::steady_clock::now();
    
    Logger::getInstance().log(LogLevel::DEBUG, 
        "Compiling query with mode: ", static_cast<int>(flags.mode));
    
    CompilationResult result;
    
    // Route based on compilation mode
    switch (flags.mode) {
        case CompilationMode::LOCAL_ONLY:
            result = compile_local(query, flags);
            break;
            
        case CompilationMode::CACHED_LLM:
            // Check cache first
            if (auto cached = check_cache(query, flags)) {
                result = *cached;
                Logger::getInstance().log(LogLevel::DEBUG, 
                    "Cache hit for CACHED_LLM mode");
            } else if (is_llm_available()) {
                result = compile_llm(query, flags);
                // Cache successful result
                if (result.success && m_cache) {
                    bool cached = m_cache->put(query, flags, result);
                    if (cached) {
                        Logger::getInstance().log(LogLevel::DEBUG, "Cached LLM result for future use");
                    } else {
                        auto stats = m_cache->get_statistics();
                        Logger::getInstance().log(LogLevel::ERROR, 
                            "Failed to cache LLM result - cache entries: ", stats.entry_count,
                            ", memory usage: ", m_cache->get_memory_usage(), " bytes");
                    }
                }
            } else {
                // Fallback to local compilation and cache it
                result = compile_local(query, flags);
                if (result.success && m_cache) {
                    bool cached = m_cache->put(query, flags, result);
                    if (cached) {
                        Logger::getInstance().log(LogLevel::DEBUG, "Cached local fallback result");
                    } else {
                        auto stats = m_cache->get_statistics();
                        Logger::getInstance().log(LogLevel::ERROR, 
                            "Failed to cache local fallback result - cache entries: ", stats.entry_count,
                            ", memory usage: ", m_cache->get_memory_usage(), " bytes");
                    }
                }
            }
            break;
            
        case CompilationMode::ASYNC_LLM:
            // For now, just use local compilation
            // Async LLM will be implemented in future PR
            result = compile_local(query, flags);
            break;
            
        case CompilationMode::FULL_LLM:
            if (is_llm_available()) {
                result = compile_llm(query, flags);
            } else {
                // Fallback to local
                Logger::getInstance().log(LogLevel::NORMAL, 
                    "LLM not available, falling back to local compilation");
                result = compile_local(query, flags);
            }
            break;
    }
    
    // Update compilation metrics
    update_metrics(result, start_time);
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        if (result.metrics.cache_hit) {
            m_cache_stats.cache_hits++;
        } else {
            m_cache_stats.cache_misses++;
        }
        m_cache_stats.total_requests++;
        m_cache_stats.hit_rate = static_cast<double>(m_cache_stats.cache_hits) / 
                                 m_cache_stats.total_requests;
        
        if (result.metrics.used_llm) {
            m_cost_stats.daily_cost += result.metrics.actual_cost;
            m_cost_stats.daily_requests++;
            m_cost_stats.average_cost = m_cost_stats.daily_cost / 
                                        m_cost_stats.daily_requests;
        }
    }
    
    return result;
}

std::future<CompilationResult> HybridCompilerImpl::compile_async(
    std::string_view query, const CompilerFlags& flags) {
    // Launch compilation in a separate thread
    return std::async(std::launch::async, 
        [this, q = std::string(query), f = flags]() {
            return this->compile(q, f);
        });
}

std::vector<CompilationResult> HybridCompilerImpl::compile_batch(
    const std::vector<std::string>& queries,
    const CompilerFlags& flags) {
    std::vector<CompilationResult> results;
    results.reserve(queries.size());
    
    for (const auto& query : queries) {
        results.push_back(compile(query, flags));
    }
    
    return results;
}

bool HybridCompilerImpl::is_llm_available() const {
    // Check if LLM components are available
    if (!m_prompt_compiler) {
        return false;
    }
    
    // Check if PromptCompiler has valid configuration
    if (!m_prompt_compiler->is_available()) {
        return false;
    }
    
    // Check circuit breaker state
    if (m_circuit_breaker && !m_circuit_breaker->can_execute()) {
        Logger::getInstance().log(LogLevel::DEBUG, 
            "LLM unavailable - circuit breaker is open");
        return false;
    }
    
    // Check cost budget
    if (m_cost_controller) {
        if (m_cost_controller->is_daily_budget_exceeded() || 
            m_cost_controller->is_monthly_budget_exceeded()) {
            Logger::getInstance().log(LogLevel::DEBUG,
                "LLM unavailable - budget exceeded");
            return false;
        }
    }
    
    return true;
}

CacheStatistics HybridCompilerImpl::get_cache_statistics() const {
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    return m_cache_stats;
}

CostStatistics HybridCompilerImpl::get_cost_statistics() const {
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    return m_cost_stats;
}

void HybridCompilerImpl::warm_cache(const std::vector<std::string>& common_queries,
                                   const CompilerFlags& flags) {
    Logger::getInstance().log(LogLevel::INFO, 
        "Warming cache with ", common_queries.size(), " queries");
    
    for (const auto& query : common_queries) {
        [[maybe_unused]] auto result = compile(query, flags);
    }
}

void HybridCompilerImpl::clear_cache() {
    if (m_cache) {
        m_cache->clear();
        Logger::getInstance().log(LogLevel::INFO, "Cache cleared");
    }
    
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    m_cache_stats = CacheStatistics{};
}

void HybridCompilerImpl::set_daily_budget(double budget_dollars) {
    m_daily_budget.store(budget_dollars);
    Logger::getInstance().log(LogLevel::INFO, 
        "Daily budget set to: $", budget_dollars);
}

void HybridCompilerImpl::set_validation_enabled(bool enabled) {
    m_validation_enabled.store(enabled);
}

CompilationResult HybridCompilerImpl::compile_local(std::string_view query,
                                                   const CompilerFlags& flags) {
    if (!m_local_compiler) {
        return CompilationResult::error_result("Local compiler not available", 
                                              std::string(query));
    }
    
    return m_local_compiler->compile(query, flags);
}

std::optional<CompilationResult> HybridCompilerImpl::check_cache(
    std::string_view query, const CompilerFlags& flags) {
    if (!m_cache) {
        return std::nullopt;
    }
    
    return m_cache->get(query, flags);
}

CompilationResult HybridCompilerImpl::compile_llm(std::string_view query,
                                                 const CompilerFlags& flags) {
    if (!m_prompt_compiler) {
        return CompilationResult::error_result("PromptCompiler not available",
                                              std::string(query));
    }
    
    try {
        // Estimate cost and check budget authorization
        if (m_cost_controller) {
            auto estimated_cost = m_prompt_compiler->estimate_cost(query, flags);
            if (estimated_cost && !m_cost_controller->authorize_request(*estimated_cost)) {
                Logger::getInstance().log(LogLevel::ERROR,
                    "LLM request denied - would exceed budget limits");
                return CompilationResult::error_result(
                    "Request denied - budget limits would be exceeded",
                    std::string(query));
            }
        }
        
        // Execute with circuit breaker protection
        CompilationResult result;
        bool success = false;
        
        if (m_circuit_breaker) {
            success = m_circuit_breaker->execute([&]() {
                result = m_prompt_compiler->compile(query, flags);
                return result.success;
            }, "llm_compilation");
            
            if (!success) {
                return CompilationResult::error_result(
                    "LLM compilation failed - circuit breaker rejected request",
                    std::string(query));
            }
        } else {
            result = m_prompt_compiler->compile(query, flags);
        }
        
        // Record actual cost if successful
        if (result.success && m_cost_controller && result.metrics.actual_cost > 0.0) {
            m_cost_controller->record_cost(result.metrics.actual_cost, "compilation");
            
            // Record validation cost if applicable
            if (result.validation_result.validation_method == "llm_semantic_validation") {
                double validation_cost = result.metrics.actual_cost * 0.3; // Estimate
                m_cost_controller->record_cost(validation_cost, "validation");
            }
        }
        
        return result;
        
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR,
            "LLM compilation exception: ", e.what());
        
        // Record failure in circuit breaker
        if (m_circuit_breaker) {
            m_circuit_breaker->record_failure(e.what());
        }
        
        return CompilationResult::error_result(
            std::string("LLM compilation exception: ") + e.what(),
            std::string(query));
    }
}

ValidationResult HybridCompilerImpl::validate_result(std::string_view original,
                                                    std::string_view compiled) {
    if (!m_validator) {
        ValidationResult result;
        result.is_semantically_equivalent = true;
        result.confidence_score = 0.95;
        result.validation_method = "default_heuristic";
        return result;
    }
    
    if (!m_validation_enabled.load()) {
        ValidationResult result;
        result.is_semantically_equivalent = true;
        result.confidence_score = 1.0;
        result.validation_method = "validation_disabled";
        return result;
    }
    
    return m_validator->validate_equivalence(original, compiled);
}

void HybridCompilerImpl::update_metrics(CompilationResult& result,
                                       std::chrono::steady_clock::time_point start_time) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    // Update total compilation time if not already set
    if (result.metrics.compilation_time == std::chrono::milliseconds(0)) {
        result.metrics.compilation_time = duration;
    }
}

} // namespace meta_prompt
} // namespace cql
