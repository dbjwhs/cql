// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "cql/meta_prompt/types.hpp"
#include "cql/meta_prompt/compiler.hpp"

/**
 * @file test_meta_prompt_foundation.cpp
 * @brief Unit tests for Meta-Prompt Compiler foundation types and interfaces
 * 
 * These tests verify that the basic types and interfaces compile correctly
 * and have expected behavior for the foundation components.
 */

namespace cql {
namespace meta_prompt {

class MetaPromptFoundationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Basic setup for foundation tests
    }
    
    void TearDown() override {
        // Cleanup after foundation tests
    }
};

// Test CompilerFlags structure
TEST_F(MetaPromptFoundationTest, CompilerFlags_DefaultValues) {
    CompilerFlags flags;
    
    EXPECT_EQ(flags.mode, CompilationMode::LOCAL_ONLY);
    EXPECT_EQ(flags.goal, OptimizationGoal::BALANCED);
    EXPECT_TRUE(flags.validate_semantics);
    EXPECT_TRUE(flags.enable_caching);
    EXPECT_FALSE(flags.use_deterministic);
    EXPECT_EQ(flags.domain, "general");
    EXPECT_DOUBLE_EQ(flags.cost_budget, 0.01);
    EXPECT_FLOAT_EQ(flags.temperature, 0.1f);
}

TEST_F(MetaPromptFoundationTest, CompilerFlags_CustomValues) {
    CompilerFlags flags{
        .mode = CompilationMode::FULL_LLM,
        .goal = OptimizationGoal::REDUCE_TOKENS,
        .validate_semantics = false,
        .domain = "system_programming",
        .cost_budget = 0.05
    };
    
    EXPECT_EQ(flags.mode, CompilationMode::FULL_LLM);
    EXPECT_EQ(flags.goal, OptimizationGoal::REDUCE_TOKENS);
    EXPECT_FALSE(flags.validate_semantics);
    EXPECT_EQ(flags.domain, "system_programming");
    EXPECT_DOUBLE_EQ(flags.cost_budget, 0.05);
}

// Test CompilationResult static factory methods
TEST_F(MetaPromptFoundationTest, CompilationResult_SuccessFactory) {
    CompilationMetrics metrics;
    metrics.compilation_time = std::chrono::milliseconds(250);
    metrics.used_llm = true;
    metrics.token_reduction_percent = 25.5f;
    
    ValidationResult validation;
    validation.is_semantically_equivalent = true;
    validation.confidence_score = 0.95;
    
    auto result = CompilationResult::success_result("optimized prompt", metrics, validation);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.compiled_prompt, "optimized prompt");
    EXPECT_EQ(result.metrics.compilation_time, std::chrono::milliseconds(250));
    EXPECT_TRUE(result.metrics.used_llm);
    EXPECT_FLOAT_EQ(result.metrics.token_reduction_percent, 25.5f);
    EXPECT_TRUE(result.validation_result.is_semantically_equivalent);
    EXPECT_DOUBLE_EQ(result.validation_result.confidence_score, 0.95);
}

TEST_F(MetaPromptFoundationTest, CompilationResult_ErrorFactory) {
    auto result = CompilationResult::error_result("API timeout", "original query");
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "API timeout");
    EXPECT_EQ(result.original_query, "original query");
    EXPECT_TRUE(result.compiled_prompt.empty());
}

TEST_F(MetaPromptFoundationTest, CompilationResult_ErrorFactory_NoOriginalQuery) {
    auto result = CompilationResult::error_result("Parse error");
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "Parse error");
    EXPECT_TRUE(result.original_query.empty());
}

// Test CompilationMode enum values
TEST_F(MetaPromptFoundationTest, CompilationMode_EnumValues) {
    // Verify all compilation modes are defined
    CompilationMode mode;
    
    mode = CompilationMode::LOCAL_ONLY;
    EXPECT_EQ(mode, CompilationMode::LOCAL_ONLY);
    
    mode = CompilationMode::CACHED_LLM;
    EXPECT_EQ(mode, CompilationMode::CACHED_LLM);
    
    mode = CompilationMode::ASYNC_LLM;
    EXPECT_EQ(mode, CompilationMode::ASYNC_LLM);
    
    mode = CompilationMode::FULL_LLM;
    EXPECT_EQ(mode, CompilationMode::FULL_LLM);
}

// Test OptimizationGoal enum values  
TEST_F(MetaPromptFoundationTest, OptimizationGoal_EnumValues) {
    OptimizationGoal goal;
    
    goal = OptimizationGoal::REDUCE_TOKENS;
    EXPECT_EQ(goal, OptimizationGoal::REDUCE_TOKENS);
    
    goal = OptimizationGoal::IMPROVE_ACCURACY;
    EXPECT_EQ(goal, OptimizationGoal::IMPROVE_ACCURACY);
    
    goal = OptimizationGoal::DOMAIN_SPECIFIC;
    EXPECT_EQ(goal, OptimizationGoal::DOMAIN_SPECIFIC);
    
    goal = OptimizationGoal::BALANCED;
    EXPECT_EQ(goal, OptimizationGoal::BALANCED);
}

// Test ValidationResult structure
TEST_F(MetaPromptFoundationTest, ValidationResult_DefaultValues) {
    ValidationResult validation;
    
    EXPECT_FALSE(validation.is_semantically_equivalent);
    EXPECT_DOUBLE_EQ(validation.confidence_score, 0.0);
    EXPECT_TRUE(validation.detected_issues.empty());
    EXPECT_TRUE(validation.validation_method.empty());
}

TEST_F(MetaPromptFoundationTest, ValidationResult_WithIssues) {
    ValidationResult validation;
    validation.is_semantically_equivalent = false;
    validation.confidence_score = 0.65;
    validation.detected_issues = {"semantic_drift", "missing_context"};
    validation.validation_method = "ast_comparison";
    
    EXPECT_FALSE(validation.is_semantically_equivalent);
    EXPECT_DOUBLE_EQ(validation.confidence_score, 0.65);
    EXPECT_EQ(validation.detected_issues.size(), 2);
    EXPECT_EQ(validation.detected_issues[0], "semantic_drift");
    EXPECT_EQ(validation.detected_issues[1], "missing_context");
    EXPECT_EQ(validation.validation_method, "ast_comparison");
}

// Test CompilationMetrics structure
TEST_F(MetaPromptFoundationTest, CompilationMetrics_DefaultValues) {
    CompilationMetrics metrics;
    
    EXPECT_EQ(metrics.compilation_time, std::chrono::milliseconds(0));
    EXPECT_EQ(metrics.llm_api_time, std::chrono::milliseconds(0));
    EXPECT_DOUBLE_EQ(metrics.estimated_cost, 0.0);
    EXPECT_DOUBLE_EQ(metrics.actual_cost, 0.0);
    EXPECT_FALSE(metrics.cache_hit);
    EXPECT_FALSE(metrics.used_llm);
    EXPECT_EQ(metrics.input_tokens, 0);
    EXPECT_EQ(metrics.output_tokens, 0);
    EXPECT_FLOAT_EQ(metrics.token_reduction_percent, 0.0f);
}

// Test CacheStatistics structure
TEST_F(MetaPromptFoundationTest, CacheStatistics_HitRateCalculation) {
    CacheStatistics stats;
    stats.total_requests = 100;
    stats.cache_hits = 75;
    stats.cache_misses = 25;
    stats.hit_rate = static_cast<double>(stats.cache_hits) / stats.total_requests;
    
    EXPECT_EQ(stats.cache_hits + stats.cache_misses, stats.total_requests);
    EXPECT_DOUBLE_EQ(stats.hit_rate, 0.75);
}

// Test CostStatistics structure
TEST_F(MetaPromptFoundationTest, CostStatistics_BudgetUtilization) {
    CostStatistics stats;
    stats.daily_cost = 7.50;
    stats.daily_requests = 150;
    stats.average_cost = stats.daily_cost / stats.daily_requests;
    
    const double budget = 10.0;
    stats.budget_utilization = stats.daily_cost / budget;
    
    EXPECT_DOUBLE_EQ(stats.average_cost, 0.05);
    EXPECT_DOUBLE_EQ(stats.budget_utilization, 0.75);
}

// Test GlobalCompilerConfig structure
TEST_F(MetaPromptFoundationTest, GlobalCompilerConfig_DefaultValues) {
    GlobalCompilerConfig config;
    
    EXPECT_TRUE(config.anthropic_api_key.empty());
    EXPECT_EQ(config.default_model, "claude-haiku-3.5");
    EXPECT_DOUBLE_EQ(config.default_daily_budget, 10.0);
    EXPECT_EQ(config.max_cache_size_mb, 100);
    EXPECT_EQ(config.cache_ttl, std::chrono::seconds(3600));
    EXPECT_TRUE(config.enable_metrics_collection);
}

// Integration test: Full CompilerFlags to CompilationResult workflow
TEST_F(MetaPromptFoundationTest, Integration_CompilerWorkflow) {
    // Setup compiler flags for a realistic scenario
    CompilerFlags flags{
        .mode = CompilationMode::CACHED_LLM,
        .goal = OptimizationGoal::REDUCE_TOKENS,
        .domain = "code_generation",
        .cost_budget = 0.02
    };
    
    // Simulate compilation metrics
    CompilationMetrics metrics;
    metrics.compilation_time = std::chrono::milliseconds(45);
    metrics.llm_api_time = std::chrono::milliseconds(35);
    metrics.estimated_cost = 0.008;
    metrics.actual_cost = 0.007;
    metrics.cache_hit = true;
    metrics.used_llm = true;
    metrics.input_tokens = 500;
    metrics.output_tokens = 350;
    metrics.token_reduction_percent = 30.0f;
    
    // Simulate validation results
    ValidationResult validation;
    validation.is_semantically_equivalent = true;
    validation.confidence_score = 0.92;
    validation.validation_method = "semantic_similarity";
    
    // Create successful result
    auto result = CompilationResult::success_result(
        "Optimized prompt with reduced tokens", metrics, validation);
    result.original_query = "Original verbose query";
    result.flags_used = flags;
    
    // Verify the complete workflow
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.compiled_prompt, "Optimized prompt with reduced tokens");
    EXPECT_EQ(result.original_query, "Original verbose query");
    EXPECT_EQ(result.flags_used.mode, CompilationMode::CACHED_LLM);
    EXPECT_EQ(result.flags_used.goal, OptimizationGoal::REDUCE_TOKENS);
    EXPECT_TRUE(result.metrics.cache_hit);
    EXPECT_TRUE(result.metrics.used_llm);
    EXPECT_LT(result.metrics.actual_cost, flags.cost_budget);
    EXPECT_GT(result.metrics.token_reduction_percent, 0.0f);
    EXPECT_TRUE(result.validation_result.is_semantically_equivalent);
    EXPECT_GT(result.validation_result.confidence_score, 0.9);
}

} // namespace meta_prompt
} // namespace cql
