// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/config.hpp"
#include "../../include/cql/project_utils.hpp"
#include <cstdlib>
#include <fstream>
#include <filesystem>

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
    
    // This is a stub implementation
    // In the full implementation, this would parse a JSON/YAML config file
    Logger::getInstance().log(LogLevel::INFO, 
        "Loading configuration from file: ", path);
    
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::getInstance().log(LogLevel::NORMAL, 
            "Could not open config file: ", path);
        return config;
    }
    
    // TODO: Implement JSON/YAML parsing
    Logger::getInstance().log(LogLevel::NORMAL, 
        "Config file parsing not yet implemented");
    
    return config;
}

Config Config::load_from_default_locations() {
    Config config;
    
    // First load from environment
    config = load_from_environment();
    
    // Then try to load from default config file locations
    std::vector<std::filesystem::path> config_paths;
    
    // Check home directory
    if (const char* home = std::getenv("HOME")) {
        config_paths.push_back(std::filesystem::path(home) / ".cql" / "config.json");
        config_paths.push_back(std::filesystem::path(home) / ".config" / "cql" / "config.json");
    }
    
    // Check current directory
    config_paths.push_back("cql.config.json");
    config_paths.push_back(".cql.json");
    
    // Try each path
    for (const auto& path : config_paths) {
        if (std::filesystem::exists(path)) {
            Logger::getInstance().log(LogLevel::INFO, 
                "Found config file at: ", path.string());
            // Merge with file config (file takes precedence)
            auto file_config = load_from_file(path.string());
            // TODO: Implement config merging
            break;
        }
    }
    
    // Set defaults if not configured
    if (config.get_default_provider().empty()) {
        config.set_default_provider("anthropic");
    }
    
    if (config.get_model("anthropic").empty()) {
        config.set_model("anthropic", "claude-3-opus-20240229");
    }
    
    return config;
}

} // namespace cql
