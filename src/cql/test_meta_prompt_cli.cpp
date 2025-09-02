// MIT License
// Copyright (c) 2025 dbjwhs

#include "gtest/gtest.h"
#include "cql/meta_prompt_handler.hpp"
#include "cql/meta_prompt/types.hpp"
#include "cql/cql.hpp"
#include <string>
#include <iostream>
#include <sstream>

namespace cql {
namespace test {

class MetaPromptCLITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Redirect cout for testing output
        original_cout = std::cout.rdbuf();
        std::cout.rdbuf(output_stream.rdbuf());
    }
    
    void TearDown() override {
        // Restore cout
        std::cout.rdbuf(original_cout);
    }
    
    std::string getOutput() {
        return output_stream.str();
    }
    
    void clearOutput() {
        output_stream.str("");
        output_stream.clear();
    }

private:
    std::ostringstream output_stream;
    std::streambuf* original_cout;
};

TEST_F(MetaPromptCLITest, ParseCompilationMode) {
    // Test valid compilation modes
    EXPECT_EQ(MetaPromptHandler::parse_compilation_mode("LOCAL_ONLY"), 
              meta_prompt::CompilationMode::LOCAL_ONLY);
    EXPECT_EQ(MetaPromptHandler::parse_compilation_mode("CACHED_LLM"), 
              meta_prompt::CompilationMode::CACHED_LLM);
    EXPECT_EQ(MetaPromptHandler::parse_compilation_mode("FULL_LLM"), 
              meta_prompt::CompilationMode::FULL_LLM);
    EXPECT_EQ(MetaPromptHandler::parse_compilation_mode("ASYNC_LLM"), 
              meta_prompt::CompilationMode::ASYNC_LLM);
    
    // Test invalid mode defaults to CACHED_LLM
    // Note: Warning goes to stderr, not cout, so we just test the return value
    EXPECT_EQ(MetaPromptHandler::parse_compilation_mode("INVALID_MODE"), 
              meta_prompt::CompilationMode::CACHED_LLM);
}

TEST_F(MetaPromptCLITest, ParseOptimizationGoal) {
    // Test valid optimization goals
    EXPECT_EQ(MetaPromptHandler::parse_optimization_goal("REDUCE_TOKENS"), 
              meta_prompt::OptimizationGoal::REDUCE_TOKENS);
    EXPECT_EQ(MetaPromptHandler::parse_optimization_goal("IMPROVE_ACCURACY"), 
              meta_prompt::OptimizationGoal::IMPROVE_ACCURACY);
    EXPECT_EQ(MetaPromptHandler::parse_optimization_goal("BALANCED"), 
              meta_prompt::OptimizationGoal::BALANCED);
    EXPECT_EQ(MetaPromptHandler::parse_optimization_goal("DOMAIN_SPECIFIC"), 
              meta_prompt::OptimizationGoal::DOMAIN_SPECIFIC);
    
    // Test invalid goal defaults to BALANCED
    // Note: Warning goes to stderr, not cout, so we just test the return value
    EXPECT_EQ(MetaPromptHandler::parse_optimization_goal("INVALID_GOAL"), 
              meta_prompt::OptimizationGoal::BALANCED);
}

TEST_F(MetaPromptCLITest, DisplayCompilationResultSuccess) {
    // Create a successful compilation result
    meta_prompt::CompilationResult result = meta_prompt::CompilationResult::success_result(
        "Optimized prompt text", 
        {},
        meta_prompt::ValidationResult{}
    );
    result.original_query = "Original prompt text that is longer";
    result.validation_result.is_semantically_equivalent = true;
    result.validation_result.confidence_score = 0.92;
    result.validation_result.validation_method = "heuristic_analysis";
    
    result.metrics.compilation_time = std::chrono::milliseconds(150);
    result.metrics.cache_hit = true;
    result.metrics.used_llm = false;
    result.metrics.token_reduction_percent = 15.0f;
    
    clearOutput();
    MetaPromptHandler::display_compilation_result(result, true, true);
    std::string output = getOutput();
    
    // Verify success status
    EXPECT_TRUE(output.find("✅ SUCCESS") != std::string::npos);
    
    // Verify metrics display
    EXPECT_TRUE(output.find("--- COMPILATION METRICS ---") != std::string::npos);
    EXPECT_TRUE(output.find("Compilation time: 150 ms") != std::string::npos);
    EXPECT_TRUE(output.find("Cache hit: Yes") != std::string::npos);
    EXPECT_TRUE(output.find("Used LLM: No") != std::string::npos);
    
    // Verify validation display
    EXPECT_TRUE(output.find("--- SEMANTIC VALIDATION ---") != std::string::npos);
    EXPECT_TRUE(output.find("Semantically equivalent: ✅ Yes") != std::string::npos);
    EXPECT_TRUE(output.find("Confidence score: 0.92") != std::string::npos);
    EXPECT_TRUE(output.find("heuristic_analysis") != std::string::npos);
}

TEST_F(MetaPromptCLITest, DisplayCompilationResultFailure) {
    // Create a failed compilation result
    meta_prompt::CompilationResult result = meta_prompt::CompilationResult::error_result(
        "Compilation failed: API unavailable",
        "Original query text"
    );
    
    clearOutput();
    MetaPromptHandler::display_compilation_result(result, false, false);
    std::string output = getOutput();
    
    // Verify failure status
    EXPECT_TRUE(output.find("❌ FAILED") != std::string::npos);
    EXPECT_TRUE(output.find("API unavailable") != std::string::npos);
}

TEST_F(MetaPromptCLITest, HandleOptimizeCommandMissingFile) {
    // Test with insufficient arguments
    const char* argv[] = {"cql", "--optimize"};
    int argc = 2;
    
    // Capture stderr for error message
    std::ostringstream error_stream;
    std::streambuf* original_cerr = std::cerr.rdbuf();
    std::cerr.rdbuf(error_stream.rdbuf());
    
    int result = MetaPromptHandler::handle_optimize_command(argc, const_cast<char**>(argv));
    
    // Restore stderr
    std::cerr.rdbuf(original_cerr);
    
    EXPECT_EQ(result, CQL_ERROR);
    EXPECT_TRUE(error_stream.str().find("Input file required") != std::string::npos);
}

// Integration test requiring actual file system
TEST_F(MetaPromptCLITest, DISABLED_HandleOptimizeCommandIntegration) {
    // This test would require setting up actual files and testing the full pipeline
    // Disabled for CI/CD but useful for local development testing
    
    const char* argv[] = {
        "cql", "--optimize", "../examples/template_example.llm", 
        "--mode", "LOCAL_ONLY", "--show-metrics"
    };
    int argc = 6;
    
    int result = MetaPromptHandler::handle_optimize_command(argc, const_cast<char**>(argv));
    EXPECT_EQ(result, CQL_NO_ERROR);
    
    std::string output = getOutput();
    EXPECT_TRUE(output.find("META-PROMPT COMPILATION RESULTS") != std::string::npos);
}

} // namespace test
} // namespace cql
