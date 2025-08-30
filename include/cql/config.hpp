// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <chrono>

namespace cql {

/**
 * @brief Comprehensive configuration management for CQL
 * 
 * This class manages all configuration settings for the CQL system,
 * including API keys, provider settings, runtime options, and file I/O.
 * Supports configuration loading from multiple sources with proper precedence:
 * CLI parameters > Environment variables > Config files > Defaults
 * 
 * Configuration file locations (checked in order):
 * - ~/.cql/config.json
 * - ~/.config/cql/config.json  
 * - ./cql.config.json
 * - ./.cql.json
 */
class Config {
public:
    Config() = default;
    ~Config() = default;
    
    // Provider configuration
    [[nodiscard]] std::string get_default_provider() const { 
        return m_default_provider; 
    }
    
    void set_default_provider(const std::string& provider) {
        m_default_provider = provider;
    }
    
    [[nodiscard]] std::vector<std::string> get_fallback_chain() const {
        return m_fallback_chain;
    }
    
    void set_fallback_chain(const std::vector<std::string>& chain) {
        m_fallback_chain = chain;
    }
    
    // API configuration
    [[nodiscard]] std::string get_api_key(const std::string& provider) const {
        auto it = m_api_keys.find(provider);
        return (it != m_api_keys.end()) ? it->second : "";
    }
    
    void set_api_key(const std::string& provider, const std::string& key) {
        m_api_keys[provider] = key;
    }
    
    // Model configuration  
    [[nodiscard]] std::string get_model(const std::string& provider) const {
        auto it = m_models.find(provider);
        return (it != m_models.end()) ? it->second : "";
    }
    
    void set_model(const std::string& provider, const std::string& model) {
        m_models[provider] = model;
    }
    
    // Temperature configuration
    [[nodiscard]] double get_temperature() const {
        return m_temperature;
    }
    
    void set_temperature(double temp) {
        m_temperature = temp;
    }
    
    // Max tokens configuration
    [[nodiscard]] int get_max_tokens() const {
        return m_max_tokens;
    }
    
    void set_max_tokens(int tokens) {
        m_max_tokens = tokens;
    }
    
    // Base URL configuration
    [[nodiscard]] std::optional<std::string> get_base_url(const std::string& provider) const {
        auto it = m_base_urls.find(provider);
        return (it != m_base_urls.end()) ? std::optional(it->second) : std::nullopt;
    }
    
    void set_base_url(const std::string& provider, const std::string& url) {
        m_base_urls[provider] = url;
    }
    
    // Timeout configuration
    [[nodiscard]] std::chrono::seconds get_timeout(const std::string& provider = "") const {
        if (!provider.empty()) {
            auto it = m_provider_timeouts.find(provider);
            if (it != m_provider_timeouts.end()) {
                return it->second;
            }
        }
        return m_default_timeout;
    }
    
    void set_timeout(std::chrono::seconds timeout, const std::string& provider = "") {
        if (provider.empty()) {
            m_default_timeout = timeout;
        } else {
            m_provider_timeouts[provider] = timeout;
        }
    }
    
    // Max retries configuration
    [[nodiscard]] int get_max_retries(const std::string& provider = "") const {
        if (!provider.empty()) {
            auto it = m_provider_retries.find(provider);
            if (it != m_provider_retries.end()) {
                return it->second;
            }
        }
        return m_default_max_retries;
    }
    
    void set_max_retries(int retries, const std::string& provider = "") {
        if (provider.empty()) {
            m_default_max_retries = retries;
        } else {
            m_provider_retries[provider] = retries;
        }
    }
    
    // Output directory configuration
    [[nodiscard]] std::string get_output_directory() const {
        return m_output_directory;
    }
    
    void set_output_directory(const std::string& dir) {
        m_output_directory = dir;
    }
    
    // Configuration validation
    [[nodiscard]] bool is_provider_configured(const std::string& provider) const;
    [[nodiscard]] bool validate_configuration() const;
    [[nodiscard]] std::vector<std::string> get_validation_errors() const;
    
    // Configuration persistence
    [[nodiscard]] bool save_to_file(const std::string& path) const;
    
    // Load configuration from various sources
    [[nodiscard]] static Config load_from_environment();
    [[nodiscard]] static Config load_from_file(const std::string& path);
    [[nodiscard]] static Config load_from_default_locations();
    
    // Merge configurations with precedence
    void merge_with(const Config& other);
    [[nodiscard]] static Config merge_configs(const Config& base, const Config& override);
    
private:
    // Core configuration
    std::string m_default_provider = "anthropic";
    std::vector<std::string> m_fallback_chain;
    std::map<std::string, std::string> m_api_keys;
    std::map<std::string, std::string> m_models;
    std::map<std::string, std::string> m_base_urls;
    
    // Generation parameters
    double m_temperature = 0.7;
    int m_max_tokens = 4096;
    
    // Network and retry configuration
    std::chrono::seconds m_default_timeout{120};
    std::map<std::string, std::chrono::seconds> m_provider_timeouts;
    int m_default_max_retries = 3;
    std::map<std::string, int> m_provider_retries;
    
    // File I/O configuration
    std::string m_output_directory = "./output";
    
    // Internal helpers
    [[nodiscard]] static std::vector<std::string> get_default_config_paths();
    void apply_defaults();
    [[nodiscard]] bool parse_json_config(const std::string& json_content);
};

/**
 * @brief Configuration profile for managing different environments
 * 
 * Allows users to define different configuration profiles (dev, staging, prod)
 * and switch between them easily.
 */
class ConfigProfile {
public:
    ConfigProfile(const std::string& name, Config config)
        : m_name(name), m_config(std::move(config)) {}
    
    [[nodiscard]] const std::string& get_name() const { return m_name; }
    [[nodiscard]] const Config& get_config() const { return m_config; }
    [[nodiscard]] Config& get_config() { return m_config; }
    
private:
    std::string m_name;
    Config m_config;
};

/**
 * @brief Configuration manager for handling multiple profiles
 */
class ConfigManager {
public:
    ConfigManager() = default;
    
    void add_profile(const std::string& name, const Config& config);
    [[nodiscard]] bool has_profile(const std::string& name) const;
    [[nodiscard]] const Config& get_profile(const std::string& name) const;
    [[nodiscard]] std::vector<std::string> list_profiles() const;
    
    void set_active_profile(const std::string& name);
    [[nodiscard]] const Config& get_active_config() const;
    [[nodiscard]] std::string get_active_profile_name() const;
    
    [[nodiscard]] static ConfigManager load_from_file(const std::string& path);
    [[nodiscard]] bool save_to_file(const std::string& path) const;
    
private:
    std::map<std::string, ConfigProfile> m_profiles;
    std::string m_active_profile = "default";
    Config m_default_config;
};

} // namespace cql
