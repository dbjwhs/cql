// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "ai_provider.hpp"

namespace cql {

// Forward declaration
class Config;

/**
 * @brief Factory for creating AI provider instances
 * 
 * This factory manages the creation and registration of AI providers.
 * It supports both built-in providers (Anthropic, OpenAI, etc.) and
 * custom provider implementations.
 * 
 * @note Thread-safe singleton implementation
 */
class ProviderFactory {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return Reference to the global ProviderFactory instance
     */
    [[nodiscard]] static ProviderFactory& get_instance();
    
    /**
     * @brief Create a provider instance
     * 
     * @param provider_name Name of the provider (e.g., "anthropic", "openai")
     * @param config Configuration for the provider
     * @return Unique pointer to the created provider
     * @throws std::invalid_argument if provider_name is unknown
     * @throws std::runtime_error if provider creation fails
     */
    [[nodiscard]] std::unique_ptr<AIProvider> create_provider(
        const std::string& provider_name,
        const Config& config
    );
    
    /**
     * @brief Register a custom provider factory function
     * 
     * Allows registration of custom provider implementations.
     * 
     * @param name Provider name for registration
     * @param factory Factory function that creates the provider
     * @throws std::invalid_argument if name is already registered
     */
    void register_provider(
        const std::string& name,
        std::function<std::unique_ptr<AIProvider>(const Config&)> factory
    );
    
    /**
     * @brief Get list of available provider names
     * 
     * @return Vector of registered provider names
     */
    [[nodiscard]] std::vector<std::string> get_available_providers() const;
    
    /**
     * @brief Check if a provider is registered
     * 
     * @param name Provider name to check
     * @return true if provider is registered
     */
    [[nodiscard]] bool has_provider(const std::string& name) const;
    
    /**
     * @brief Create provider based on configuration
     * 
     * Creates a provider using the default_provider setting in config.
     * 
     * @param config Configuration containing default_provider setting
     * @return Unique pointer to the created provider
     * @throws std::runtime_error if no default provider configured
     */
    [[nodiscard]] std::unique_ptr<AIProvider> create_from_config(const Config& config);
    
    /**
     * @brief Create a provider chain for fallback
     * 
     * Creates multiple providers based on fallback_chain configuration.
     * If the first provider fails, the next one in the chain is tried.
     * 
     * @param config Configuration containing fallback_chain setting
     * @return Vector of provider instances in fallback order
     */
    [[nodiscard]] std::vector<std::unique_ptr<AIProvider>> create_fallback_chain(
        const Config& config
    );

private:
    // Private constructor for singleton
    ProviderFactory();
    ~ProviderFactory() = default;
    
    // Delete copy and move operations
    ProviderFactory(const ProviderFactory&) = delete;
    ProviderFactory& operator=(const ProviderFactory&) = delete;
    ProviderFactory(ProviderFactory&&) = delete;
    ProviderFactory& operator=(ProviderFactory&&) = delete;
    
    // Registry of provider factory functions
    using FactoryFunction = std::function<std::unique_ptr<AIProvider>(const Config&)>;
    std::unordered_map<std::string, FactoryFunction> m_factories;
    
    // Initialize built-in providers
    void register_builtin_providers();
};

/**
 * @brief Helper class for automatic provider registration
 * 
 * Use this class to automatically register providers at program startup.
 * 
 * @example
 * @code
 * // In implementation file:
 * static ProviderRegistrar register_my_provider("my_provider", 
 *     [](const Config& cfg) { return std::make_unique<MyProvider>(cfg); });
 * @endcode
 */
class ProviderRegistrar {
public:
    /**
     * @brief Register a provider at construction
     * 
     * @param name Provider name
     * @param factory Factory function for the provider
     */
    ProviderRegistrar(
        const std::string& name,
        std::function<std::unique_ptr<AIProvider>(const Config&)> factory
    );
};

} // namespace cql
