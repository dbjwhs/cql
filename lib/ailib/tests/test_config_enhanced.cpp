// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "ailib/core/config.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace cql::test {

class EnhancedConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        m_temp_dir = std::filesystem::temp_directory_path() / "cql_config_test";
        std::filesystem::create_directories(m_temp_dir);
    }
    
    void TearDown() override {
        // Clean up temporary files
        if (std::filesystem::exists(m_temp_dir)) {
            std::filesystem::remove_all(m_temp_dir);
        }
        
        // Clear test environment variables
        unsetenv("CQL_API_KEY");
        unsetenv("CQL_DEFAULT_PROVIDER");
        unsetenv("CQL_MODEL");
        unsetenv("CQL_TEMPERATURE");
        unsetenv("CQL_MAX_TOKENS");
    }
    
    void create_test_config_file(const std::string& content) {
        m_test_config_path = m_temp_dir / "test_config.json";
        std::ofstream file(m_test_config_path);
        file << content;
        file.close();
    }
    
    std::filesystem::path m_temp_dir;
    std::filesystem::path m_test_config_path;
};

TEST_F(EnhancedConfigTest, JSONConfigurationParsing) {
    std::string config_json = R"({
        "default_provider": "anthropic",
        "temperature": 0.8,
        "max_tokens": 8192,
        "output_directory": "/tmp/test_output",
        "default_timeout": 180,
        "default_max_retries": 5,
        "fallback_chain": ["anthropic", "openai"],
        "providers": {
            "anthropic": {
                "api_key": "sk-test-key-anthropic-123456789",
                "model": "claude-3-sonnet-20240229",
                "base_url": "https://api.anthropic.com",
                "timeout": 120,
                "max_retries": 3
            },
            "openai": {
                "api_key": "sk-test-key-openai-987654321",
                "model": "gpt-4-turbo",
                "base_url": "https://api.openai.com/v1",
                "timeout": 90,
                "max_retries": 2
            }
        }
    })";
    
    create_test_config_file(config_json);
    auto config = Config::load_from_file(m_test_config_path.string());
    
    // Test core configuration
    EXPECT_EQ(config.get_default_provider(), "anthropic");
    EXPECT_DOUBLE_EQ(config.get_temperature(), 0.8);
    EXPECT_EQ(config.get_max_tokens(), 8192);
    EXPECT_EQ(config.get_output_directory(), "/tmp/test_output");
    EXPECT_EQ(config.get_timeout().count(), 180);
    EXPECT_EQ(config.get_max_retries(), 5);
    
    // Test fallback chain
    auto fallback_chain = config.get_fallback_chain();
    EXPECT_EQ(fallback_chain.size(), 2);
    EXPECT_EQ(fallback_chain[0], "anthropic");
    EXPECT_EQ(fallback_chain[1], "openai");
    
    // Test provider-specific configuration
    EXPECT_EQ(config.get_api_key("anthropic"), "sk-test-key-anthropic-123456789");
    EXPECT_EQ(config.get_model("anthropic"), "claude-3-sonnet-20240229");
    EXPECT_EQ(config.get_base_url("anthropic").value(), "https://api.anthropic.com");
    EXPECT_EQ(config.get_timeout("anthropic").count(), 120);
    EXPECT_EQ(config.get_max_retries("anthropic"), 3);
    
    EXPECT_EQ(config.get_api_key("openai"), "sk-test-key-openai-987654321");
    EXPECT_EQ(config.get_model("openai"), "gpt-4-turbo");
    EXPECT_EQ(config.get_base_url("openai").value(), "https://api.openai.com/v1");
    EXPECT_EQ(config.get_timeout("openai").count(), 90);
    EXPECT_EQ(config.get_max_retries("openai"), 2);
}

TEST_F(EnhancedConfigTest, ConfigurationValidation) {
    Config valid_config;
    valid_config.set_api_key("anthropic", "sk-test-key-1234567890abcdef");
    valid_config.set_model("anthropic", "claude-3-sonnet-20240229");
    valid_config.set_temperature(0.7);
    valid_config.set_max_tokens(4096);
    
    EXPECT_TRUE(valid_config.validate_configuration());
    EXPECT_TRUE(valid_config.is_provider_configured("anthropic"));
    
    auto errors = valid_config.get_validation_errors();
    EXPECT_TRUE(errors.empty());
    
    // Test invalid configuration
    Config invalid_config;
    invalid_config.set_temperature(3.0); // Invalid temperature
    invalid_config.set_max_tokens(-100);  // Invalid max_tokens
    
    EXPECT_FALSE(invalid_config.validate_configuration());
    
    auto invalid_errors = invalid_config.get_validation_errors();
    EXPECT_FALSE(invalid_errors.empty());
    EXPECT_GE(invalid_errors.size(), 2); // Should have at least 2 errors
}

TEST_F(EnhancedConfigTest, ConfigurationMerging) {
    Config base_config;
    base_config.set_default_provider("anthropic");
    base_config.set_temperature(0.7);
    base_config.set_max_tokens(4096);
    base_config.set_api_key("anthropic", "base-key");
    
    Config override_config;
    override_config.set_temperature(0.9);
    override_config.set_api_key("anthropic", "override-key");
    override_config.set_api_key("openai", "openai-key");
    
    base_config.merge_with(override_config);
    
    // Check that overrides took precedence
    EXPECT_DOUBLE_EQ(base_config.get_temperature(), 0.9);
    EXPECT_EQ(base_config.get_api_key("anthropic"), "override-key");
    EXPECT_EQ(base_config.get_api_key("openai"), "openai-key");
    
    // Check that non-overridden values remained
    EXPECT_EQ(base_config.get_default_provider(), "anthropic");
    EXPECT_EQ(base_config.get_max_tokens(), 4096);
}

TEST_F(EnhancedConfigTest, ConfigurationPersistence) {
    Config config;
    config.set_default_provider("anthropic");
    config.set_temperature(0.8);
    config.set_max_tokens(8192);
    config.set_api_key("anthropic", "sk-test-key-12345");
    config.set_model("anthropic", "claude-3-sonnet-20240229");
    config.set_base_url("anthropic", "https://api.anthropic.com");
    config.set_timeout(std::chrono::seconds(150));
    config.set_max_retries(4);
    config.set_output_directory("/tmp/test");
    
    std::string save_path = (m_temp_dir / "saved_config.json").string();
    
    // Save configuration
    EXPECT_TRUE(config.save_to_file(save_path));
    EXPECT_TRUE(std::filesystem::exists(save_path));
    
    // Load configuration and verify
    auto loaded_config = Config::load_from_file(save_path);
    
    EXPECT_EQ(loaded_config.get_default_provider(), "anthropic");
    EXPECT_DOUBLE_EQ(loaded_config.get_temperature(), 0.8);
    EXPECT_EQ(loaded_config.get_max_tokens(), 8192);
    EXPECT_EQ(loaded_config.get_api_key("anthropic"), "sk-test-key-12345");
    EXPECT_EQ(loaded_config.get_model("anthropic"), "claude-3-sonnet-20240229");
    EXPECT_EQ(loaded_config.get_base_url("anthropic").value(), "https://api.anthropic.com");
    EXPECT_EQ(loaded_config.get_timeout().count(), 150);
    EXPECT_EQ(loaded_config.get_max_retries(), 4);
    EXPECT_EQ(loaded_config.get_output_directory(), "/tmp/test");
}

TEST_F(EnhancedConfigTest, EnvironmentVariableLoading) {
    // Set environment variables
    setenv("CQL_API_KEY", "env-api-key-123", 1);
    setenv("CQL_DEFAULT_PROVIDER", "openai", 1);
    setenv("CQL_MODEL", "gpt-4", 1);
    setenv("CQL_TEMPERATURE", "0.9", 1);
    setenv("CQL_MAX_TOKENS", "8000", 1);
    
    auto config = Config::load_from_environment();
    
    EXPECT_EQ(config.get_api_key("anthropic"), "env-api-key-123");
    EXPECT_EQ(config.get_default_provider(), "openai");
    EXPECT_EQ(config.get_model("openai"), "gpt-4");
    EXPECT_DOUBLE_EQ(config.get_temperature(), 0.9);
    EXPECT_EQ(config.get_max_tokens(), 8000);
}

TEST_F(EnhancedConfigTest, DefaultLocationsPrecedence) {
    // Create config file
    std::string config_json = R"({
        "default_provider": "anthropic",
        "temperature": 0.6,
        "providers": {
            "anthropic": {
                "api_key": "file-api-key",
                "model": "claude-3-haiku-20240307"
            }
        }
    })";
    
    create_test_config_file(config_json);
    
    // Set environment variables (should override file)
    setenv("CQL_TEMPERATURE", "0.8", 1);
    setenv("CQL_API_KEY", "env-api-key", 1);
    
    // Since we can't easily test the default locations without modifying the system,
    // we'll test the precedence logic by loading separately and merging
    auto file_config = Config::load_from_file(m_test_config_path.string());
    auto env_config = Config::load_from_environment();
    
    file_config.merge_with(env_config);
    
    // Environment should override file
    EXPECT_DOUBLE_EQ(file_config.get_temperature(), 0.8);
    EXPECT_EQ(file_config.get_api_key("anthropic"), "env-api-key");
    EXPECT_EQ(file_config.get_model("anthropic"), "claude-3-haiku-20240307"); // From file
}

TEST_F(EnhancedConfigTest, ConfigManager) {
    ConfigManager manager;
    
    Config dev_config;
    dev_config.set_default_provider("anthropic");
    dev_config.set_temperature(0.9);
    dev_config.set_api_key("anthropic", "dev-key");
    
    Config prod_config;
    prod_config.set_default_provider("anthropic");
    prod_config.set_temperature(0.1);
    prod_config.set_api_key("anthropic", "prod-key");
    
    manager.add_profile("dev", dev_config);
    manager.add_profile("prod", prod_config);
    
    EXPECT_TRUE(manager.has_profile("dev"));
    EXPECT_TRUE(manager.has_profile("prod"));
    EXPECT_FALSE(manager.has_profile("staging"));
    
    auto profiles = manager.list_profiles();
    EXPECT_EQ(profiles.size(), 2);
    
    manager.set_active_profile("dev");
    EXPECT_EQ(manager.get_active_profile_name(), "dev");
    
    const auto& active_config = manager.get_active_config();
    EXPECT_DOUBLE_EQ(active_config.get_temperature(), 0.9);
    EXPECT_EQ(active_config.get_api_key("anthropic"), "dev-key");
    
    manager.set_active_profile("prod");
    const auto& prod_active = manager.get_active_config();
    EXPECT_DOUBLE_EQ(prod_active.get_temperature(), 0.1);
    EXPECT_EQ(prod_active.get_api_key("anthropic"), "prod-key");
}

TEST_F(EnhancedConfigTest, ProviderSpecificTimeouts) {
    Config config;
    config.set_timeout(std::chrono::seconds(120)); // Default timeout
    config.set_timeout(std::chrono::seconds(180), "anthropic");
    config.set_timeout(std::chrono::seconds(60), "openai");
    
    EXPECT_EQ(config.get_timeout().count(), 120);
    EXPECT_EQ(config.get_timeout("anthropic").count(), 180);
    EXPECT_EQ(config.get_timeout("openai").count(), 60);
    EXPECT_EQ(config.get_timeout("unknown").count(), 120); // Should return default
}

TEST_F(EnhancedConfigTest, InvalidJSONHandling) {
    std::string invalid_json = R"({
        "default_provider": "anthropic",
        "temperature": 0.8,
        "invalid_json": true
        // Missing closing brace
    })";
    
    create_test_config_file(invalid_json);
    auto config = Config::load_from_file(m_test_config_path.string());
    
    // Should return default configuration when JSON is invalid
    EXPECT_EQ(config.get_default_provider(), "anthropic"); // Default value
    EXPECT_DOUBLE_EQ(config.get_temperature(), 0.7); // Default value
}

} // namespace cql::test
