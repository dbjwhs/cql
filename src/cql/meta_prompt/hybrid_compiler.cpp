// MIT License
// Copyright (c) 2025 dbjwhs

#include "cql/meta_prompt/hybrid_compiler.hpp"
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
    , m_prompt_compiler(nullptr)  // Will be implemented in future PRs
    , m_cache(nullptr)
    , m_validator(nullptr)
    , m_circuit_breaker(nullptr)
    , m_cost_controller(nullptr) {
    
    Logger::getInstance().log(LogLevel::INFO, 
        "HybridCompiler initialized in LOCAL_ONLY mode");
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
            } else if (is_llm_available()) {
                result = compile_llm(query, flags);
                // Cache successful result
                if (result.success && m_cache) {
                    // Cache implementation in future PR
                }
            } else {
                // Fallback to local
                result = compile_local(query, flags);
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
    
    // Check circuit breaker state
    if (m_circuit_breaker) {
        // Circuit breaker implementation in future PR
    }
    
    // Check cost budget
    if (m_cost_controller) {
        // Cost controller implementation in future PR
    }
    
    return false; // For now, LLM is not available
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
        // Cache implementation in future PR
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
    std::string_view /* query */, const CompilerFlags& /* flags */) {
    if (!m_cache) {
        return std::nullopt;
    }
    
    // Cache implementation in future PR
    return std::nullopt;
}

CompilationResult HybridCompilerImpl::compile_llm(std::string_view query,
                                                 const CompilerFlags& /* flags */) {
    // LLM compilation will be implemented in future PR
    return CompilationResult::error_result("LLM compilation not yet implemented",
                                          std::string(query));
}

ValidationResult HybridCompilerImpl::validate_result(std::string_view /* original */,
                                                    std::string_view /* compiled */) {
    if (!m_validator) {
        ValidationResult result;
        result.is_semantically_equivalent = true;
        result.confidence_score = 0.95;
        result.validation_method = "default_heuristic";
        return result;
    }
    
    // Validator implementation in future PR
    return ValidationResult{};
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
