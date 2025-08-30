// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/config.hpp"
#include "../../include/cql/project_utils.hpp"
#include "../../include/cql/json_utils.hpp"
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <sstream>

namespace cql {

Config Config::load_from_environment() {
    Config config;
    
    // Load from environment variables
    if (const char* api_key = std::getenv("CQL_API_KEY")) {
        config.set_api_key("anthropic", api_key);
        Logger::getInstance().log(LogLevel::INFO, 
            "Loaded API key from CQL_API_KEY environment variable");
    }
    
    if (const char* provider = std::getenv("CQL_DEFAULT_PROVIDER")) {
        config.set_default_provider(provider);
        Logger::getInstance().log(LogLevel::INFO, 
            "Default provider set to: ", provider);
    }
    
    if (const char* model = std::getenv("CQL_MODEL")) {
        config.set_model(config.get_default_provider(), model);
        Logger::getInstance().log(LogLevel::INFO, 
            "Model set to: ", model);
    }
    
    if (const char* temp_str = std::getenv("CQL_TEMPERATURE")) {
        try {
            double temp = std::stod(temp_str);
            config.set_temperature(temp);
            Logger::getInstance().log(LogLevel::INFO, 
                "Temperature set to: ", temp);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::NORMAL, 
                "Invalid temperature value: ", temp_str);
        }
    }
    
    if (const char* tokens_str = std::getenv("CQL_MAX_TOKENS")) {
        try {
            int tokens = std::stoi(tokens_str);
            config.set_max_tokens(tokens);
            Logger::getInstance().log(LogLevel::INFO, 
                "Max tokens set to: ", tokens);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::NORMAL, 
                "Invalid max_tokens value: ", tokens_str);
        }
    }
    
    return config;
}

Config Config::load_from_file(const std::string& path) {
    Config config;
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Loading configuration from file: ", path);
    
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::getInstance().log(LogLevel::NORMAL, 
            "Could not open config file: ", path);
        return config;
    }
    
    try {
        // Read entire file content
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        std::string json_content = buffer.str();
        if (json_content.empty()) {
            Logger::getInstance().log(LogLevel::NORMAL, 
                "Config file is empty: ", path);
            return config;
        }
        
        // Parse JSON configuration
        if (!config.parse_json_config(json_content)) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "Failed to parse config file: ", path);
            return config;
        }
        
        Logger::getInstance().log(LogLevel::INFO, 
            "Successfully loaded configuration from: ", path);
            
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Error loading config file ", path, ": ", e.what());
    }
    
    return config;
}

Config Config::load_from_default_locations() {
    Logger::getInstance().log(LogLevel::DEBUG, 
        "Loading configuration from default locations");
    
    // Start with defaults
    Config config;
    config.apply_defaults();
    
    // Try to load from config files (lower precedence)
    auto config_paths = get_default_config_paths();
    for (const auto& path : config_paths) {
        if (std::filesystem::exists(path)) {
            Logger::getInstance().log(LogLevel::INFO, 
                "Found config file at: ", path);
            auto file_config = load_from_file(path);
            config.merge_with(file_config);
            break; // Use first found config file
        }
    }
    
    // Load from environment variables (higher precedence)
    auto env_config = load_from_environment();
    config.merge_with(env_config);
    
    Logger::getInstance().log(LogLevel::DEBUG, 
        "Configuration loading completed from default locations");
    
    return config;
}

// Configuration validation methods
bool Config::is_provider_configured(const std::string& provider) const {
    Logger::getInstance().log(LogLevel::DEBUG, 
        "Checking if provider '", provider, "' is configured");
    
    // Check if API key exists and is valid
    std::string api_key = get_api_key(provider);
    if (api_key.empty() || api_key.length() < 10) {
        return false;
    }
    
    // Check if model is specified
    std::string model = get_model(provider);
    if (model.empty()) {
        return false;
    }
    
    return true;
}

bool Config::validate_configuration() const {
    Logger::getInstance().log(LogLevel::DEBUG, "Validating configuration");
    
    // Check if at least one provider is configured
    if (!is_provider_configured(m_default_provider)) {
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Default provider '", m_default_provider, "' is not properly configured");
        return false;
    }
    
    // Validate temperature range
    if (m_temperature < 0.0 || m_temperature > 2.0) {
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Temperature out of valid range: ", m_temperature);
        return false;
    }
    
    // Validate max_tokens
    if (m_max_tokens <= 0 || m_max_tokens > 200000) {
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Max tokens out of valid range: ", m_max_tokens);
        return false;
    }
    
    return true;
}

std::vector<std::string> Config::get_validation_errors() const {
    std::vector<std::string> errors;
    
    if (!is_provider_configured(m_default_provider)) {
        errors.push_back("Default provider '" + m_default_provider + "' is not properly configured");
    }
    
    if (m_temperature < 0.0 || m_temperature > 2.0) {
        errors.push_back("Temperature must be between 0.0 and 2.0, got: " + std::to_string(m_temperature));
    }
    
    if (m_max_tokens <= 0 || m_max_tokens > 200000) {
        errors.push_back("Max tokens must be between 1 and 200000, got: " + std::to_string(m_max_tokens));
    }
    
    return errors;
}

// Configuration persistence
bool Config::save_to_file(const std::string& path) const {
    Logger::getInstance().log(LogLevel::INFO, "Saving configuration to: ", path);
    
    try {
        nlohmann::json config_json;
        
        // Core configuration
        config_json["default_provider"] = m_default_provider;
        config_json["fallback_chain"] = m_fallback_chain;
        config_json["temperature"] = m_temperature;
        config_json["max_tokens"] = m_max_tokens;
        config_json["output_directory"] = m_output_directory;
        config_json["default_timeout"] = m_default_timeout.count();
        config_json["default_max_retries"] = m_default_max_retries;
        
        // Provider-specific settings
        nlohmann::json providers;
        std::set<std::string> all_providers;
        
        // Collect all provider names
        for (const auto& [provider, _] : m_api_keys) all_providers.insert(provider);
        for (const auto& [provider, _] : m_models) all_providers.insert(provider);
        for (const auto& [provider, _] : m_base_urls) all_providers.insert(provider);
        for (const auto& [provider, _] : m_provider_timeouts) all_providers.insert(provider);
        for (const auto& [provider, _] : m_provider_retries) all_providers.insert(provider);
        
        for (const auto& provider : all_providers) {
            nlohmann::json provider_config;
            
            auto api_key = get_api_key(provider);
            if (!api_key.empty()) {
                provider_config["api_key"] = api_key;
            }
            
            auto model = get_model(provider);
            if (!model.empty()) {
                provider_config["model"] = model;
            }
            
            auto base_url = get_base_url(provider);
            if (base_url) {
                provider_config["base_url"] = base_url.value();
            }
            
            auto timeout = get_timeout(provider);
            if (timeout != m_default_timeout) {
                provider_config["timeout"] = timeout.count();
            }
            
            auto retries = get_max_retries(provider);
            if (retries != m_default_max_retries) {
                provider_config["max_retries"] = retries;
            }
            
            if (!provider_config.empty()) {
                providers[provider] = provider_config;
            }
        }
        
        if (!providers.empty()) {
            config_json["providers"] = providers;
        }
        
        // Create directory if needed
        std::filesystem::path config_path(path);
        std::filesystem::create_directories(config_path.parent_path());
        
        // Write to file
        std::ofstream file(path);
        if (!file.is_open()) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "Could not open config file for writing: ", path);
            return false;
        }
        
        file << config_json.dump(2); // Pretty print with 2-space indent
        file.close();
        
        Logger::getInstance().log(LogLevel::INFO, 
            "Configuration saved successfully to: ", path);
        return true;
        
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Error saving config file ", path, ": ", e.what());
        return false;
    }
}

// Configuration merging
void Config::merge_with(const Config& other) {
    Logger::getInstance().log(LogLevel::DEBUG, "Merging configuration");
    
    // Only override if other has non-default values
    if (other.m_default_provider != "anthropic") {
        m_default_provider = other.m_default_provider;
    }
    
    if (!other.m_fallback_chain.empty()) {
        m_fallback_chain = other.m_fallback_chain;
    }
    
    // Merge maps
    for (const auto& [provider, key] : other.m_api_keys) {
        if (!key.empty()) {
            m_api_keys[provider] = key;
        }
    }
    
    for (const auto& [provider, model] : other.m_models) {
        if (!model.empty()) {
            m_models[provider] = model;
        }
    }
    
    for (const auto& [provider, url] : other.m_base_urls) {
        if (!url.empty()) {
            m_base_urls[provider] = url;
        }
    }
    
    for (const auto& [provider, timeout] : other.m_provider_timeouts) {
        m_provider_timeouts[provider] = timeout;
    }
    
    for (const auto& [provider, retries] : other.m_provider_retries) {
        m_provider_retries[provider] = retries;
    }
    
    // Override scalars if they differ from defaults
    if (other.m_temperature != 0.7) {
        m_temperature = other.m_temperature;
    }
    
    if (other.m_max_tokens != 4096) {
        m_max_tokens = other.m_max_tokens;
    }
    
    if (other.m_default_timeout != std::chrono::seconds(120)) {
        m_default_timeout = other.m_default_timeout;
    }
    
    if (other.m_default_max_retries != 3) {
        m_default_max_retries = other.m_default_max_retries;
    }
    
    if (other.m_output_directory != "./output") {
        m_output_directory = other.m_output_directory;
    }
}

Config Config::merge_configs(const Config& base, const Config& override) {
    Config result = base;
    result.merge_with(override);
    return result;
}

// Internal helper methods
std::vector<std::string> Config::get_default_config_paths() {
    std::vector<std::string> paths;
    
    // Check home directory
    if (const char* home = std::getenv("HOME")) {
        paths.push_back((std::filesystem::path(home) / ".cql" / "config.json").string());
        paths.push_back((std::filesystem::path(home) / ".config" / "cql" / "config.json").string());
    }
    
    // Check current directory
    paths.push_back("cql.config.json");
    paths.push_back(".cql.json");
    
    return paths;
}

void Config::apply_defaults() {
    Logger::getInstance().log(LogLevel::DEBUG, "Applying default configuration values");
    
    if (m_default_provider.empty()) {
        m_default_provider = "anthropic";
    }
    
    // Set default model for anthropic if not set
    if (m_models.find("anthropic") == m_models.end()) {
        m_models["anthropic"] = "claude-3-sonnet-20240229";
    }
}

bool Config::parse_json_config(const std::string& json_content) {
    try {
        auto json_obj = nlohmann::json::parse(json_content);
        
        Logger::getInstance().log(LogLevel::DEBUG, "Parsing JSON configuration");
        
        // Parse core settings
        if (json_obj.contains("default_provider")) {
            m_default_provider = json_obj["default_provider"].get<std::string>();
        }
        
        if (json_obj.contains("fallback_chain")) {
            m_fallback_chain = json_obj["fallback_chain"].get<std::vector<std::string>>();
        }
        
        if (json_obj.contains("temperature")) {
            m_temperature = json_obj["temperature"].get<double>();
        }
        
        if (json_obj.contains("max_tokens")) {
            m_max_tokens = json_obj["max_tokens"].get<int>();
        }
        
        if (json_obj.contains("output_directory")) {
            m_output_directory = json_obj["output_directory"].get<std::string>();
        }
        
        if (json_obj.contains("default_timeout")) {
            m_default_timeout = std::chrono::seconds(json_obj["default_timeout"].get<int>());
        }
        
        if (json_obj.contains("default_max_retries")) {
            m_default_max_retries = json_obj["default_max_retries"].get<int>();
        }
        
        // Parse provider-specific settings
        if (json_obj.contains("providers")) {
            const auto& providers = json_obj["providers"];
            
            for (auto& [provider_name, provider_config] : providers.items()) {
                Logger::getInstance().log(LogLevel::DEBUG, 
                    "Parsing configuration for provider: ", provider_name);
                
                if (provider_config.contains("api_key")) {
                    m_api_keys[provider_name] = provider_config["api_key"].get<std::string>();
                }
                
                if (provider_config.contains("model")) {
                    m_models[provider_name] = provider_config["model"].get<std::string>();
                }
                
                if (provider_config.contains("base_url")) {
                    m_base_urls[provider_name] = provider_config["base_url"].get<std::string>();
                }
                
                if (provider_config.contains("timeout")) {
                    m_provider_timeouts[provider_name] = std::chrono::seconds(
                        provider_config["timeout"].get<int>());
                }
                
                if (provider_config.contains("max_retries")) {
                    m_provider_retries[provider_name] = provider_config["max_retries"].get<int>();
                }
            }
        }
        
        Logger::getInstance().log(LogLevel::DEBUG, "JSON configuration parsed successfully");
        return true;
        
    } catch (const nlohmann::json::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "JSON parsing error: ", e.what());
        return false;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Configuration parsing error: ", e.what());
        return false;
    }
}

// ConfigManager implementation
void ConfigManager::add_profile(const std::string& name, const Config& config) {
    Logger::getInstance().log(LogLevel::DEBUG, "Adding configuration profile: ", name);
    m_profiles.emplace(name, ConfigProfile(name, config));
}

bool ConfigManager::has_profile(const std::string& name) const {
    return m_profiles.find(name) != m_profiles.end();
}

const Config& ConfigManager::get_profile(const std::string& name) const {
    auto it = m_profiles.find(name);
    if (it == m_profiles.end()) {
        Logger::getInstance().log(LogLevel::NORMAL, 
            "Profile '", name, "' not found, returning default");
        return m_default_config;
    }
    return it->second.get_config();
}

std::vector<std::string> ConfigManager::list_profiles() const {
    std::vector<std::string> profile_names;
    for (const auto& [name, _] : m_profiles) {
        profile_names.push_back(name);
    }
    return profile_names;
}

void ConfigManager::set_active_profile(const std::string& name) {
    if (!has_profile(name)) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Cannot set active profile '", name, "': profile does not exist");
        return;
    }
    
    m_active_profile = name;
    Logger::getInstance().log(LogLevel::INFO, 
        "Active profile set to: ", name);
}

const Config& ConfigManager::get_active_config() const {
    return get_profile(m_active_profile);
}

std::string ConfigManager::get_active_profile_name() const {
    return m_active_profile;
}

ConfigManager ConfigManager::load_from_file(const std::string& path) {
    ConfigManager manager;
    Logger::getInstance().log(LogLevel::INFO, 
        "Loading configuration manager from: ", path);
    
    // Implementation would parse a configuration file with multiple profiles
    // For now, just load a single default profile
    auto config = Config::load_from_file(path);
    manager.add_profile("default", config);
    manager.m_default_config = config;
    
    return manager;
}

bool ConfigManager::save_to_file(const std::string& path) const {
    Logger::getInstance().log(LogLevel::INFO, 
        "Saving configuration manager to: ", path);
    
    // For now, just save the active config
    return get_active_config().save_to_file(path);
}

} // namespace cql
