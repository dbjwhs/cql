// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "cql/meta_prompt/hybrid_compiler.hpp"
#include "cql/meta_prompt/types.hpp"
#include <thread>
#include <chrono>

/**
 * @file test_hybrid_compiler.cpp
 * @brief Unit tests for HybridCompiler implementation
 * 
 * Tests the concrete implementation of the meta-prompt compiler,
 * focusing on LOCAL_ONLY mode for this PR.
 */

namespace cql {
namespace meta_prompt {

class HybridCompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize compiler system with test configuration
        GlobalCompilerConfig config;
        config.anthropic_api_key = "test-key";
        config.default_daily_budget = 1.0;
        config.enable_metrics_collection = true;
        [[maybe_unused]] auto init_result = initialize_compiler_system(config);
        
        // Create compiler instance
        compiler = HybridCompiler::create();
    }
    
    void TearDown() override {
        compiler.reset();
        shutdown_compiler_system();
    }
    
    std::unique_ptr<HybridCompiler> compiler;
};

// Test basic compilation in LOCAL_ONLY mode
TEST_F(HybridCompilerTest, LocalOnlyCompilation) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY,
        .goal = OptimizationGoal::BALANCED
    };
    
    std::string query = "@description \"Create a simple counter class\"";
    auto result = compiler->compile(query, flags);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.compiled_prompt.empty());
    EXPECT_EQ(result.flags_used.mode, CompilationMode::LOCAL_ONLY);
    EXPECT_FALSE(result.metrics.used_llm);
    EXPECT_FALSE(result.metrics.cache_hit);
    EXPECT_LT(result.metrics.compilation_time, std::chrono::milliseconds(10));
}

// Test token reduction optimization
TEST_F(HybridCompilerTest, TokenReductionOptimization) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY,
        .goal = OptimizationGoal::REDUCE_TOKENS
    };
    
    // Query with extra whitespace
    std::string query = "  @description   \"Create   a   simple   counter\"  \n\n  ";
    auto result = compiler->compile(query, flags);
    
    EXPECT_TRUE(result.success);
    EXPECT_LT(result.compiled_prompt.length(), query.length());
    EXPECT_GT(result.metrics.token_reduction_percent, 0.0f);
}

// Test accuracy improvement optimization
TEST_F(HybridCompilerTest, AccuracyImprovementOptimization) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY,
        .goal = OptimizationGoal::IMPROVE_ACCURACY
    };
    
    std::string query = "@description \"Create a function\"";
    auto result = compiler->compile(query, flags);
    
    EXPECT_TRUE(result.success);
    // Should add clarity markers
    EXPECT_GT(result.compiled_prompt.length(), query.length());
    EXPECT_NE(result.compiled_prompt.find("precise"), std::string::npos);
}

// Test domain-specific optimization
TEST_F(HybridCompilerTest, DomainSpecificOptimization) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY,
        .goal = OptimizationGoal::DOMAIN_SPECIFIC,
        .domain = "code_generation"
    };
    
    std::string query = "@description \"Create a logger\"";
    auto result = compiler->compile(query, flags);
    
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.compiled_prompt.find("production-ready"), std::string::npos);
}

// Test error handling
TEST_F(HybridCompilerTest, ErrorHandling) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    // Empty query should still work
    std::string query = "";
    auto result = compiler->compile(query, flags);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.compiled_prompt.empty());
}

// Test async compilation
TEST_F(HybridCompilerTest, AsyncCompilation) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    std::string query = "@description \"Async test\"";
    auto future = compiler->compile_async(query, flags);
    
    // Should complete quickly for LOCAL_ONLY
    auto status = future.wait_for(std::chrono::milliseconds(100));
    EXPECT_EQ(status, std::future_status::ready);
    
    auto result = future.get();
    EXPECT_TRUE(result.success);
}

// Test batch compilation
TEST_F(HybridCompilerTest, BatchCompilation) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    std::vector<std::string> queries = {
        "@description \"Query 1\"",
        "@description \"Query 2\"",
        "@description \"Query 3\""
    };
    
    auto results = compiler->compile_batch(queries, flags);
    
    EXPECT_EQ(results.size(), 3);
    for (const auto& result : results) {
        EXPECT_TRUE(result.success);
        EXPECT_FALSE(result.metrics.used_llm);
    }
}

// Test statistics tracking
TEST_F(HybridCompilerTest, StatisticsTracking) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    // Compile a few queries
    for (int i = 0; i < 5; ++i) {
        [[maybe_unused]] auto result = compiler->compile("@description \"Test\"", flags);
    }
    
    auto cache_stats = compiler->get_cache_statistics();
    EXPECT_EQ(cache_stats.total_requests, 5);
    EXPECT_EQ(cache_stats.cache_misses, 5);
    EXPECT_EQ(cache_stats.cache_hits, 0); // No cache yet
    
    auto cost_stats = compiler->get_cost_statistics();
    EXPECT_EQ(cost_stats.daily_requests, 0); // No LLM requests
    EXPECT_DOUBLE_EQ(cost_stats.daily_cost, 0.0);
}

// Test configuration
TEST_F(HybridCompilerTest, Configuration) {
    // Test budget setting
    compiler->set_daily_budget(5.0);
    // Budget is stored but not enforced in LOCAL_ONLY mode
    
    // Test validation setting
    compiler->set_validation_enabled(false);
    
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    auto result = compiler->compile("@description \"Test\"", flags);
    EXPECT_TRUE(result.success);
}

// Test cache warming
TEST_F(HybridCompilerTest, CacheWarming) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    std::vector<std::string> common_queries = {
        "@description \"Common query 1\"",
        "@description \"Common query 2\""
    };
    
    // Should not crash even without cache implementation
    compiler->warm_cache(common_queries, flags);
    
    auto stats = compiler->get_cache_statistics();
    EXPECT_GE(stats.total_requests, 2);
}

// Test clear cache
TEST_F(HybridCompilerTest, ClearCache) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    [[maybe_unused]] auto test_result = compiler->compile("@description \"Test\"", flags);
    compiler->clear_cache();
    
    auto stats = compiler->get_cache_statistics();
    EXPECT_EQ(stats.total_requests, 0);
    EXPECT_EQ(stats.cache_hits, 0);
    EXPECT_EQ(stats.cache_misses, 0);
}

// Test LLM availability check
TEST_F(HybridCompilerTest, LLMAvailability) {
    // LLM components are now implemented and available
    EXPECT_TRUE(compiler->is_llm_available());
}

// Test performance requirements
TEST_F(HybridCompilerTest, PerformanceRequirements) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    std::string query = "@description \"Performance test with longer query content "
                       "that includes multiple directives and requirements\"";
    
    auto start = std::chrono::steady_clock::now();
    auto result = compiler->compile(query, flags);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_TRUE(result.success);
    // LOCAL_ONLY should complete in < 10ms
    EXPECT_LT(duration, std::chrono::milliseconds(10));
    EXPECT_EQ(result.metrics.compilation_time, duration);
}

// Test validation result
TEST_F(HybridCompilerTest, ValidationResult) {
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY,
        .validate_semantics = true
    };
    
    auto result = compiler->compile("@description \"Test\"", flags);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.validation_result.is_semantically_equivalent);
    EXPECT_DOUBLE_EQ(result.validation_result.confidence_score, 1.0);
    EXPECT_EQ(result.validation_result.validation_method, "local_ast_comparison");
}

// Test with custom configuration
TEST_F(HybridCompilerTest, CustomConfiguration) {
    std::unordered_map<std::string, std::string> config;
    config["mode"] = "local";
    config["cache_size"] = "100";
    
    auto custom_compiler = HybridCompiler::create_with_config(config);
    EXPECT_NE(custom_compiler, nullptr);
    
    CompilerFlags flags{
        .mode = CompilationMode::LOCAL_ONLY
    };
    
    auto result = custom_compiler->compile("@description \"Test\"", flags);
    EXPECT_TRUE(result.success);
}

} // namespace meta_prompt
} // namespace cql
