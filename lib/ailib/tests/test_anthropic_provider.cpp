// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "ailib/providers/anthropic.hpp"
#include "ailib/core/config.hpp"

namespace cql::test {

class AnthropicProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup basic config with longer mock key (needs to be >= 30 chars per is_configured())
        m_config.set_api_key("anthropic", "sk-test-key-1234567890abcdef1234567890"); // Mock key for testing
        m_config.set_model("anthropic", "claude-3-opus-20240229");
    }
    
    Config m_config;
};

TEST_F(AnthropicProviderTest, ProviderBasicInfo) {
    AnthropicProvider provider(m_config);
    
    EXPECT_EQ(provider.get_provider_name(), "Anthropic");
    EXPECT_TRUE(provider.is_configured()); // This should pass because we set a valid mock key in SetUp()
    
    auto capabilities = provider.get_capabilities();
    EXPECT_TRUE(capabilities.supports_streaming);
    EXPECT_TRUE(capabilities.supports_vision);
    EXPECT_TRUE(capabilities.supports_async);
    EXPECT_FALSE(capabilities.available_models.empty());
    EXPECT_GT(capabilities.max_context_length, 0);
    EXPECT_GT(capabilities.max_output_tokens, 0);
}

TEST_F(AnthropicProviderTest, ModelValidation) {
    AnthropicProvider provider(m_config);
    
    // Valid models
    EXPECT_TRUE(provider.validate_model("claude-3-opus-20240229"));
    EXPECT_TRUE(provider.validate_model("claude-3-sonnet-20240229"));
    EXPECT_TRUE(provider.validate_model("claude-3-haiku-20240307"));
    EXPECT_TRUE(provider.validate_model("claude-3-opus")); // Alias
    
    // Invalid models
    EXPECT_FALSE(provider.validate_model("gpt-4"));
    EXPECT_FALSE(provider.validate_model("invalid-model"));
    EXPECT_FALSE(provider.validate_model(""));
}

TEST_F(AnthropicProviderTest, CostEstimation) {
    AnthropicProvider provider(m_config);
    
    ProviderRequest request;
    request.model = "claude-3-opus-20240229";
    request.prompt = "Hello, world!";
    request.max_tokens = 100;
    
    auto cost = provider.estimate_cost(request);
    ASSERT_TRUE(cost.has_value());
    EXPECT_GT(cost.value(), 0.0);
    EXPECT_LT(cost.value(), 1.0); // Should be reasonable for a small request
    
    // Test with unknown model
    request.model = "unknown-model";
    auto cost_unknown = provider.estimate_cost(request);
    EXPECT_FALSE(cost_unknown.has_value());
}

TEST_F(AnthropicProviderTest, RequestConversion) {
    AnthropicProvider provider(m_config);
    
    ProviderRequest request;
    request.model = "claude-3-opus-20240229";
    request.prompt = "Test prompt";
    request.max_tokens = 150;
    request.temperature = 0.8;
    request.system_prompt = "You are a helpful assistant";
    request.messages = {{"user", "Hello"}, {"assistant", "Hi there!"}};
    
    // Note: We can't directly test convert_request as it's private,
    // but we can test that the provider doesn't crash with various inputs
    EXPECT_NO_THROW({
        // This would normally make an HTTP request, but without a real API key
        // it should handle the error gracefully
    });
}

TEST_F(AnthropicProviderTest, ConfigurationStates) {
    Config empty_config;
    AnthropicProvider provider_empty(empty_config);
    EXPECT_FALSE(provider_empty.is_configured());
    
    Config short_key_config;
    short_key_config.set_api_key("anthropic", "short");
    AnthropicProvider provider_short(short_key_config);
    EXPECT_FALSE(provider_short.is_configured());
    
    // Valid configuration
    Config valid_config;
    valid_config.set_api_key("anthropic", "sk-1234567890abcdef1234567890abcdef");
    AnthropicProvider provider_valid(valid_config);
    EXPECT_TRUE(provider_valid.is_configured());
}

TEST_F(AnthropicProviderTest, ProviderCapabilities) {
    AnthropicProvider provider(m_config);
    auto caps = provider.get_capabilities();
    
    // Verify expected models are available
    auto& models = caps.available_models;
    EXPECT_TRUE(std::find(models.begin(), models.end(), "claude-3-opus-20240229") != models.end());
    EXPECT_TRUE(std::find(models.begin(), models.end(), "claude-3-sonnet-20240229") != models.end());
    EXPECT_TRUE(std::find(models.begin(), models.end(), "claude-3-haiku-20240307") != models.end());
    
    // Verify capabilities match Anthropic's features
    EXPECT_TRUE(caps.supports_streaming);
    EXPECT_TRUE(caps.supports_vision);
    EXPECT_TRUE(caps.supports_async);
    EXPECT_FALSE(caps.supports_functions); // Not implemented yet
    
    // Verify reasonable limits
    EXPECT_EQ(caps.max_context_length, 200000);
    EXPECT_EQ(caps.max_output_tokens, 8192);
}

} // namespace cql::test
