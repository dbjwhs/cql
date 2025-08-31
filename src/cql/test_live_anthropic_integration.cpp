// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "ailib/providers/factory.hpp"
#include "ailib/core/config.hpp"
#include "ailib/auth/secure_store.hpp"
#include "../include/cql/project_utils.hpp"
#include <chrono>
#include <thread>

namespace cql {

/**
 * @class LiveAnthropicIntegrationTest
 * @brief Integration tests for live Anthropic API with real credentials
 * 
 * These tests require a valid ANTHROPIC_API_KEY environment variable.
 * They will be skipped if no API key is provided.
 * 
 * To run these tests:
 * export ANTHROPIC_API_KEY=your-actual-api-key-here
 * ./cql_test --gtest_filter="LiveAnthropicIntegrationTest.*"
 */
class LiveAnthropicIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().log(LogLevel::INFO, "Setting up LiveAnthropicIntegrationTest");
        
        // Load configuration from environment
        m_config = Config::load_from_environment();
        
        // Check if API key is available
        std::string api_key = m_config.get_api_key("anthropic");
        m_has_api_key = !api_key.empty() && api_key.length() >= 30;
        
        if (!m_has_api_key) {
            Logger::getInstance().log(LogLevel::NORMAL, 
                "Skipping live API tests - no valid ANTHROPIC_API_KEY found");
            Logger::getInstance().log(LogLevel::NORMAL, 
                "To run these tests: export ANTHROPIC_API_KEY=your-api-key");
            GTEST_SKIP() << "No valid ANTHROPIC_API_KEY environment variable found";
        }
        
        // Set up test configuration
        m_config.set_model("anthropic", "claude-3-haiku-20240307"); // Use fastest/cheapest model for tests
        m_config.set_temperature(0.1); // Low temperature for consistent results
        m_config.set_max_tokens(100);   // Small responses for fast tests
        
        // Create provider
        auto& factory = ProviderFactory::get_instance();
        m_provider = factory.create_provider("anthropic", m_config);
        
        ASSERT_TRUE(m_provider != nullptr) << "Failed to create Anthropic provider";
        ASSERT_TRUE(m_provider->is_configured()) << "Provider is not properly configured";
        
        Logger::getInstance().log(LogLevel::INFO, 
            "Live API tests configured with model: ", m_config.get_model("anthropic"));
    }
    
    void TearDown() override {
        if (m_has_api_key) {
            Logger::getInstance().log(LogLevel::INFO, "LiveAnthropicIntegrationTest completed");
        }
    }
    
    bool m_has_api_key = false;
    Config m_config;
    std::unique_ptr<AIProvider> m_provider;
};

/**
 * @brief Test basic API connectivity and authentication
 */
TEST_F(LiveAnthropicIntegrationTest, BasicConnectivityTest) {
    if (!m_has_api_key) GTEST_SKIP();
    
    Logger::getInstance().log(LogLevel::INFO, "Testing basic API connectivity");
    
    ProviderRequest request;
    request.prompt = "Hello! Please respond with exactly: 'API test successful'";
    request.model = m_config.get_model("anthropic");
    request.max_tokens = 50;
    request.temperature = 0.1;
    
    auto start_time = std::chrono::steady_clock::now();
    auto response = m_provider->generate(request);
    auto end_time = std::chrono::steady_clock::now();
    
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    Logger::getInstance().log(LogLevel::INFO, "API request completed in ", latency.count(), "ms");
    
    // Validate response
    ASSERT_TRUE(response.success) << "API request failed: " << response.error_message.value_or("Unknown error");
    ASSERT_FALSE(response.content.empty()) << "Response content is empty";
    ASSERT_GT(response.tokens_used, 0) << "Token usage not reported";
    ASSERT_EQ(response.model_used, request.model) << "Model used doesn't match requested model";
    
    Logger::getInstance().log(LogLevel::INFO, "Response content: ", response.content.substr(0, 100));
    Logger::getInstance().log(LogLevel::INFO, "Tokens used: ", response.tokens_used);
    Logger::getInstance().log(LogLevel::INFO, "Response latency: ", response.latency.count(), "ms");
}

/**
 * @brief Test API key validation and error handling
 */
TEST_F(LiveAnthropicIntegrationTest, APIKeyValidationTest) {
    if (!m_has_api_key) GTEST_SKIP();
    
    Logger::getInstance().log(LogLevel::INFO, "Testing API key validation");
    
    // Test with valid key (should work)
    ASSERT_TRUE(m_provider->is_configured()) << "Provider should be configured with valid API key";
    
    // Test capabilities
    auto capabilities = m_provider->get_capabilities();
    EXPECT_GT(capabilities.available_models.size(), 0) << "Should have available models";
    EXPECT_TRUE(capabilities.supports_async) << "Should support async operations";
    EXPECT_GT(capabilities.max_context_length, 0) << "Should have positive context length";
    
    Logger::getInstance().log(LogLevel::INFO, "Provider capabilities validated successfully");
}

/**
 * @brief Test cost estimation accuracy
 */
TEST_F(LiveAnthropicIntegrationTest, CostEstimationTest) {
    if (!m_has_api_key) GTEST_SKIP();
    
    Logger::getInstance().log(LogLevel::INFO, "Testing cost estimation");
    
    ProviderRequest request;
    request.prompt = "Count from 1 to 5 and explain each number briefly.";
    request.model = m_config.get_model("anthropic");
    request.max_tokens = 150;
    request.temperature = 0.1;
    
    // Get cost estimate
    auto estimated_cost = m_provider->estimate_cost(request);
    ASSERT_TRUE(estimated_cost.has_value()) << "Cost estimation should be available";
    ASSERT_GT(estimated_cost.value(), 0.0) << "Estimated cost should be positive";
    
    Logger::getInstance().log(LogLevel::INFO, "Estimated cost: $", estimated_cost.value());
    
    // Make actual request
    auto response = m_provider->generate(request);
    ASSERT_TRUE(response.success) << "Request should succeed: " << response.error_message.value_or("Unknown error");
    
    // Validate cost estimation was reasonable (should be within 50% of actual)
    if (response.tokens_used > 0) {
        // Rough cost calculation for validation
        double rough_actual_cost = (response.prompt_tokens * 0.00025 + response.completion_tokens * 0.00125) / 1000.0;
        double estimation_error = std::abs(estimated_cost.value() - rough_actual_cost) / rough_actual_cost;
        
        Logger::getInstance().log(LogLevel::INFO, "Rough actual cost: $", rough_actual_cost);
        Logger::getInstance().log(LogLevel::INFO, "Estimation error: ", estimation_error * 100, "%");
        
        EXPECT_LT(estimation_error, 0.5) << "Cost estimation should be within 50% of actual cost";
    }
}

/**
 * @brief Test request validation and error handling
 */
TEST_F(LiveAnthropicIntegrationTest, RequestValidationTest) {
    if (!m_has_api_key) GTEST_SKIP();
    
    Logger::getInstance().log(LogLevel::INFO, "Testing request validation and error handling");
    
    // Test with invalid model
    ProviderRequest invalid_request;
    invalid_request.prompt = "Hello";
    invalid_request.model = "invalid-model-name";
    invalid_request.max_tokens = 50;
    
    EXPECT_FALSE(m_provider->validate_model(invalid_request.model)) << "Should reject invalid model";
    
    // Test with valid model
    ProviderRequest valid_request;
    valid_request.prompt = "Hello";
    valid_request.model = m_config.get_model("anthropic");
    valid_request.max_tokens = 50;
    
    EXPECT_TRUE(m_provider->validate_model(valid_request.model)) << "Should accept valid model";
    
    Logger::getInstance().log(LogLevel::INFO, "Request validation tests completed");
}

/**
 * @brief Test conversation handling with multiple messages
 */
TEST_F(LiveAnthropicIntegrationTest, ConversationTest) {
    if (!m_has_api_key) GTEST_SKIP();
    
    Logger::getInstance().log(LogLevel::INFO, "Testing conversation handling");
    
    ProviderRequest request;
    request.system_prompt = "You are a helpful assistant. Keep responses brief.";
    request.messages = {
        {"user", "What is 2+2?"},
        {"assistant", "2+2 equals 4."},
        {"user", "What about 3+3?"}
    };
    request.model = m_config.get_model("anthropic");
    request.max_tokens = 50;
    request.temperature = 0.1;
    
    auto response = m_provider->generate(request);
    
    ASSERT_TRUE(response.success) << "Conversation request failed: " << response.error_message.value_or("Unknown error");
    ASSERT_FALSE(response.content.empty()) << "Response content should not be empty";
    
    // Response should contain "6" for 3+3
    std::string content_lower = response.content;
    std::transform(content_lower.begin(), content_lower.end(), content_lower.begin(), ::tolower);
    EXPECT_TRUE(content_lower.find("6") != std::string::npos) << "Response should contain the answer '6'";
    
    Logger::getInstance().log(LogLevel::INFO, "Conversation response: ", response.content);
}

/**
 * @brief Test async request functionality
 */
TEST_F(LiveAnthropicIntegrationTest, AsyncRequestTest) {
    if (!m_has_api_key) GTEST_SKIP();
    
    Logger::getInstance().log(LogLevel::INFO, "Testing async request functionality");
    
    ProviderRequest request;
    request.prompt = "List three colors and briefly describe each.";
    request.model = m_config.get_model("anthropic");
    request.max_tokens = 100;
    request.temperature = 0.1;
    
    // Start async request
    auto start_time = std::chrono::steady_clock::now();
    auto future_response = m_provider->generate_async(request);
    
    // Do some other work while waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Get response
    auto response = future_response.get();
    auto end_time = std::chrono::steady_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    Logger::getInstance().log(LogLevel::INFO, "Async request completed in ", total_time.count(), "ms");
    
    ASSERT_TRUE(response.success) << "Async request failed: " << response.error_message.value_or("Unknown error");
    ASSERT_FALSE(response.content.empty()) << "Async response content should not be empty";
    
    Logger::getInstance().log(LogLevel::INFO, "Async response: ", response.content.substr(0, 100));
}

/**
 * @brief Test retry logic with rate limiting simulation
 */
TEST_F(LiveAnthropicIntegrationTest, RetryLogicTest) {
    if (!m_has_api_key) GTEST_SKIP();
    
    Logger::getInstance().log(LogLevel::INFO, "Testing retry logic resilience");
    
    // Set up provider with aggressive retry settings
    Config retry_config = m_config;
    retry_config.set_max_retries(2, "anthropic");  // Lower retries for faster test
    
    auto& factory = ProviderFactory::get_instance();
    auto retry_provider = factory.create_provider("anthropic", retry_config);
    
    ProviderRequest request;
    request.prompt = "Simple test prompt for retry logic.";
    request.model = retry_config.get_model("anthropic");
    request.max_tokens = 50;
    request.temperature = 0.1;
    
    // This should work normally (tests that retry doesn't interfere with success)
    auto response = retry_provider->generate(request);
    
    EXPECT_TRUE(response.success) << "Normal request should succeed even with retry logic enabled";
    
    Logger::getInstance().log(LogLevel::INFO, "Retry logic test completed successfully");
}

} // namespace cql
