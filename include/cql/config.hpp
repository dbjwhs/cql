// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace cql {

/**
 * @brief Configuration management for CQL
 * 
 * This class manages configuration settings for the CQL system,
 * including API keys, provider settings, and runtime options.
 * 
 * @note This is a stub implementation that will be expanded
 *       in the Configuration Management phase.
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
    
    // Load configuration from various sources
    [[nodiscard]] static Config load_from_environment();
    [[nodiscard]] static Config load_from_file(const std::string& path);
    [[nodiscard]] static Config load_from_default_locations();
    
private:
    std::string m_default_provider = "anthropic";
    std::vector<std::string> m_fallback_chain;
    std::map<std::string, std::string> m_api_keys;
    std::map<std::string, std::string> m_models;
    std::map<std::string, std::string> m_base_urls;
    double m_temperature = 0.7;
    int m_max_tokens = 4096;
};

} // namespace cql
