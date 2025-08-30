// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/provider_factory.hpp"
#include "../../include/cql/config.hpp"
#include "../../include/cql/project_utils.hpp"
#include <stdexcept>
#include <algorithm>

namespace cql {

// Singleton instance getter
ProviderFactory& ProviderFactory::get_instance() {
    static ProviderFactory instance;
    return instance;
}

// Private constructor
ProviderFactory::ProviderFactory() {
    register_builtin_providers();
}

// Create a provider by name
std::unique_ptr<AIProvider> ProviderFactory::create_provider(
    const std::string& provider_name,
    const Config& config) {
    
    // Convert to lowercase for case-insensitive matching
    std::string lower_name = provider_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    auto it = m_factories.find(lower_name);
    if (it == m_factories.end()) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Unknown provider: ", provider_name);
        throw std::invalid_argument("Unknown provider: " + provider_name);
    }
    
    try {
        Logger::getInstance().log(LogLevel::INFO, 
            "Creating provider: ", provider_name);
        auto provider = it->second(config);
        
        if (!provider) {
            throw std::runtime_error("Provider factory returned null");
        }
        
        if (!provider->is_configured()) {
            Logger::getInstance().log(LogLevel::NORMAL, 
                "Provider ", provider_name, " is not properly configured");
        }
        
        return provider;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Failed to create provider ", provider_name, ": ", e.what());
        throw std::runtime_error("Failed to create provider " + provider_name + ": " + e.what());
    }
}

// Register a custom provider
void ProviderFactory::register_provider(
    const std::string& name,
    std::function<std::unique_ptr<AIProvider>(const Config&)> factory) {
    
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    if (m_factories.find(lower_name) != m_factories.end()) {
        throw std::invalid_argument("Provider already registered: " + name);
    }
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Registering provider: ", name);
    m_factories[lower_name] = std::move(factory);
}

// Get list of available providers
std::vector<std::string> ProviderFactory::get_available_providers() const {
    std::vector<std::string> providers;
    providers.reserve(m_factories.size());
    
    for (const auto& [name, _] : m_factories) {
        providers.push_back(name);
    }
    
    std::sort(providers.begin(), providers.end());
    return providers;
}

// Check if provider is registered
bool ProviderFactory::has_provider(const std::string& name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    return m_factories.find(lower_name) != m_factories.end();
}

// Create provider from config's default setting
std::unique_ptr<AIProvider> ProviderFactory::create_from_config(const Config& config) {
    std::string default_provider = config.get_default_provider();
    
    if (default_provider.empty()) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "No default provider configured");
        throw std::runtime_error("No default provider configured");
    }
    
    return create_provider(default_provider, config);
}

// Create fallback chain of providers
std::vector<std::unique_ptr<AIProvider>> ProviderFactory::create_fallback_chain(
    const Config& config) {
    
    std::vector<std::string> chain = config.get_fallback_chain();
    std::vector<std::unique_ptr<AIProvider>> providers;
    
    if (chain.empty()) {
        // If no fallback chain configured, just use default provider
        providers.push_back(create_from_config(config));
        return providers;
    }
    
    providers.reserve(chain.size());
    
    for (const auto& provider_name : chain) {
        try {
            providers.push_back(create_provider(provider_name, config));
            Logger::getInstance().log(LogLevel::INFO, 
                "Added ", provider_name, " to fallback chain");
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::NORMAL, 
                "Failed to add ", provider_name, " to fallback chain: ", e.what());
            // Continue with other providers in the chain
        }
    }
    
    if (providers.empty()) {
        throw std::runtime_error("Failed to create any providers in fallback chain");
    }
    
    return providers;
}

// Register built-in providers
void ProviderFactory::register_builtin_providers() {
    // Note: These will be implemented in separate files as we create each provider
    // For now, we'll add a placeholder that wraps the existing ApiClient
    
    // This will be moved to anthropic_provider.cpp once we create it
    register_provider("anthropic", [](const Config& config) -> std::unique_ptr<AIProvider> {
        // Placeholder - will be replaced with actual AnthropicProvider
        Logger::getInstance().log(LogLevel::INFO, 
            "Creating Anthropic provider (placeholder implementation)");
        Logger::getInstance().log(LogLevel::DEBUG,
            "Config received - API key configured: ", !config.get_api_key("anthropic").empty());
        
        // TODO: This is a placeholder that throws. Will be replaced with:
        // return std::make_unique<AnthropicProvider>(config);
        throw std::runtime_error("Anthropic provider implementation pending - see Phase 1.3");
    });
    
    // Future providers will be registered here
    // register_provider("openai", ...);
    // register_provider("google", ...);
    // register_provider("cohere", ...);
}

// ProviderRegistrar implementation
ProviderRegistrar::ProviderRegistrar(
    const std::string& name,
    std::function<std::unique_ptr<AIProvider>(const Config&)> factory) {
    
    ProviderFactory::get_instance().register_provider(name, std::move(factory));
}

} // namespace cql
