// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "ailib/providers/openai.hpp"
#include "ailib/providers/factory.hpp"
#include "ailib/core/config.hpp"

namespace cql::test {

class OpenAIProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_config.set_api_key("openai", "sk-test-openai-key-1234567890abcdef");
        m_config.set_model("openai", "gpt-4o");
    }

    Config m_config;
};

TEST_F(OpenAIProviderTest, ProviderBasicInfo) {
    OpenAIProvider provider(m_config);

    EXPECT_EQ(provider.get_provider_name(), "OpenAI");
    EXPECT_TRUE(provider.is_configured());

    auto capabilities = provider.get_capabilities();
    EXPECT_TRUE(capabilities.supports_streaming);
    EXPECT_TRUE(capabilities.supports_functions);
    EXPECT_TRUE(capabilities.supports_vision);
    EXPECT_TRUE(capabilities.supports_async);
    EXPECT_FALSE(capabilities.available_models.empty());
    EXPECT_GT(capabilities.max_context_length, 0);
    EXPECT_GT(capabilities.max_output_tokens, 0);
}

TEST_F(OpenAIProviderTest, ModelValidation) {
    OpenAIProvider provider(m_config);

    // Valid models
    EXPECT_TRUE(provider.validate_model("gpt-4o"));
    EXPECT_TRUE(provider.validate_model("gpt-4o-mini"));
    EXPECT_TRUE(provider.validate_model("gpt-4-turbo"));
    EXPECT_TRUE(provider.validate_model("gpt-3.5-turbo"));
    EXPECT_TRUE(provider.validate_model("o1"));
    EXPECT_TRUE(provider.validate_model("o3-mini"));

    // Invalid models
    EXPECT_FALSE(provider.validate_model("claude-3-opus"));
    EXPECT_FALSE(provider.validate_model("invalid-model"));
    EXPECT_FALSE(provider.validate_model(""));
}

TEST_F(OpenAIProviderTest, CostEstimation) {
    OpenAIProvider provider(m_config);

    ProviderRequest request;
    request.model = "gpt-4o";
    request.prompt = "Hello, world!";
    request.max_tokens = 100;

    auto cost = provider.estimate_cost(request);
    ASSERT_TRUE(cost.has_value());
    EXPECT_GT(cost.value(), 0.0);
    EXPECT_LT(cost.value(), 10.0);

    // Test with unknown model
    request.model = "unknown-model";
    auto cost_unknown = provider.estimate_cost(request);
    EXPECT_FALSE(cost_unknown.has_value());
}

TEST_F(OpenAIProviderTest, ConfigurationStates) {
    Config empty_config;
    OpenAIProvider provider_empty(empty_config);
    EXPECT_FALSE(provider_empty.is_configured());

    Config short_key_config;
    short_key_config.set_api_key("openai", "short");
    OpenAIProvider provider_short(short_key_config);
    EXPECT_FALSE(provider_short.is_configured());

    Config valid_config;
    valid_config.set_api_key("openai", "sk-1234567890abcdef12345678");
    OpenAIProvider provider_valid(valid_config);
    EXPECT_TRUE(provider_valid.is_configured());
}

TEST_F(OpenAIProviderTest, ProviderCapabilities) {
    OpenAIProvider provider(m_config);
    auto caps = provider.get_capabilities();

    auto& models = caps.available_models;
    EXPECT_TRUE(std::find(models.begin(), models.end(), "gpt-4o") != models.end());
    EXPECT_TRUE(std::find(models.begin(), models.end(), "gpt-4o-mini") != models.end());
    EXPECT_TRUE(std::find(models.begin(), models.end(), "gpt-3.5-turbo") != models.end());

    EXPECT_TRUE(caps.supports_streaming);
    EXPECT_TRUE(caps.supports_vision);
    EXPECT_TRUE(caps.supports_async);
    EXPECT_TRUE(caps.supports_functions);

    EXPECT_EQ(caps.max_context_length, 128000);
    EXPECT_EQ(caps.max_output_tokens, 16384);
}

TEST_F(OpenAIProviderTest, FactoryRegistration) {
    auto& factory = ProviderFactory::get_instance();
    EXPECT_TRUE(factory.has_provider("openai"));
    EXPECT_TRUE(factory.has_provider("anthropic"));

    auto providers = factory.get_available_providers();
    EXPECT_GE(providers.size(), 2);
}

} // namespace cql::test
