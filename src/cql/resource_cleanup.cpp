// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/resource_cleanup.hpp"
#include "../../include/cql/project_utils.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <thread>

namespace cql {

ResourceCleanup::~ResourceCleanup() {
    if (m_auto_cleanup) {
        cleanup_now();
    }
}

void ResourceCleanup::register_temp_file(const std::filesystem::path& filepath) {
    std::string path_str = filepath.string();
    if (m_registered_paths.find(path_str) == m_registered_paths.end()) {
        m_cleanup_items.emplace_back(filepath, CleanupItem::FILE_TYPE);
        m_registered_paths.insert(path_str);
        
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Registered temporary file for cleanup: ", filepath.string());
    }
}

void ResourceCleanup::register_temp_directory(const std::filesystem::path& dirpath, bool recursive) {
    std::string path_str = dirpath.string();
    if (m_registered_paths.find(path_str) == m_registered_paths.end()) {
        m_cleanup_items.emplace_back(dirpath, CleanupItem::DIRECTORY_TYPE, recursive);
        m_registered_paths.insert(path_str);
        
        Logger::getInstance().log(LogLevel::DEBUG,
            "Registered temporary directory for cleanup: ", dirpath.string(),
            " (recursive: ", recursive ? "true" : "false", ")");
    }
}

void ResourceCleanup::register_cleanup_function(CleanupFunction cleanup_func) {
    if (cleanup_func) {
        m_cleanup_items.emplace_back(std::move(cleanup_func));
        
        Logger::getInstance().log(LogLevel::DEBUG,
            "Registered custom cleanup function");
    }
}

std::size_t ResourceCleanup::cleanup_now() {
    std::size_t cleaned_count = 0;
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "Starting cleanup of ", m_cleanup_items.size(), " registered items");
    
    for (const auto& item : m_cleanup_items) {
        try {
            switch (item.type) {
                case CleanupItem::FILE_TYPE: {
                    if (std::filesystem::exists(item.path) && std::filesystem::is_regular_file(item.path)) {
                        std::filesystem::remove(item.path);
                        cleaned_count++;
                        Logger::getInstance().log(LogLevel::DEBUG,
                            "Cleaned up temporary file: ", item.path.string());
                    }
                    break;
                }
                
                case CleanupItem::DIRECTORY_TYPE: {
                    if (std::filesystem::exists(item.path) && std::filesystem::is_directory(item.path)) {
                        if (item.recursive) {
                            std::filesystem::remove_all(item.path);
                        } else {
                            std::filesystem::remove(item.path);
                        }
                        cleaned_count++;
                        Logger::getInstance().log(LogLevel::DEBUG,
                            "Cleaned up temporary directory: ", item.path.string());
                    }
                    break;
                }
                
                case CleanupItem::FUNCTION_TYPE: {
                    if (item.function) {
                        item.function();
                        cleaned_count++;
                        Logger::getInstance().log(LogLevel::DEBUG,
                            "Executed custom cleanup function");
                    }
                    break;
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR,
                "Error during cleanup: ", e.what());
        }
    }
    
    // Clear the registry after cleanup
    m_cleanup_items.clear();
    m_registered_paths.clear();
    
    if (cleaned_count > 0) {
        Logger::getInstance().log(LogLevel::INFO,
            "Resource cleanup completed - cleaned ", cleaned_count, " items");
    }
    
    return cleaned_count;
}

void ResourceCleanup::clear_registry() {
    m_cleanup_items.clear();
    m_registered_paths.clear();
    Logger::getInstance().log(LogLevel::DEBUG, "Cleared resource cleanup registry");
}

std::size_t ResourceCleanup::registered_count() const {
    return m_cleanup_items.size();
}

std::filesystem::path ResourceCleanup::create_temp_file(
    const std::filesystem::path& directory,
    const std::string& prefix,
    const std::string& extension) {
    
    // Ensure directory exists
    std::filesystem::create_directories(directory);
    
    std::filesystem::path temp_file;
    int attempts = 0;
    const int max_attempts = 100;
    
    do {
        std::string filename = generate_unique_name(prefix) + extension;
        temp_file = directory / filename;
        attempts++;
    } while (std::filesystem::exists(temp_file) && attempts < max_attempts);
    
    if (attempts >= max_attempts) {
        throw std::runtime_error("Failed to generate unique temporary filename after " + 
                                std::to_string(max_attempts) + " attempts");
    }
    
    // Create empty file
    std::ofstream ofs(temp_file);
    if (!ofs) {
        throw std::runtime_error("Failed to create temporary file: " + temp_file.string());
    }
    ofs.close();
    
    // Register for cleanup
    register_temp_file(temp_file);
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "Created temporary file: ", temp_file.string());
    
    return temp_file;
}

std::filesystem::path ResourceCleanup::create_temp_directory(
    const std::filesystem::path& parent_directory,
    const std::string& prefix) {
    
    // Ensure parent directory exists
    std::filesystem::create_directories(parent_directory);
    
    std::filesystem::path temp_dir;
    int attempts = 0;
    const int max_attempts = 100;
    
    do {
        std::string dirname = generate_unique_name(prefix);
        temp_dir = parent_directory / dirname;
        attempts++;
    } while (std::filesystem::exists(temp_dir) && attempts < max_attempts);
    
    if (attempts >= max_attempts) {
        throw std::runtime_error("Failed to generate unique temporary directory name after " + 
                                std::to_string(max_attempts) + " attempts");
    }
    
    // Create directory
    std::filesystem::create_directories(temp_dir);
    
    // Register for cleanup
    register_temp_directory(temp_dir, true);
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "Created temporary directory: ", temp_dir.string());
    
    return temp_dir;
}

std::string ResourceCleanup::generate_unique_name(const std::string& prefix) const {
    // Generate timestamp-based unique name with random component
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;
    
    // Add random component for additional uniqueness
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::ostringstream oss;
    oss << prefix << time_t << "_" << std::setfill('0') << std::setw(3) << ms 
        << "_" << dis(gen);
    
    return oss.str();
}

//=============================================================================
// TempFile implementation
//=============================================================================

TempFile::TempFile(const std::filesystem::path& directory, 
                  const std::string& prefix, 
                  const std::string& extension) {
    // Ensure directory exists
    std::filesystem::create_directories(directory);
    
    int attempts = 0;
    const int max_attempts = 100;
    
    do {
        // Generate unique filename
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        std::ostringstream oss;
        oss << prefix << time_t << "_" << std::setfill('0') << std::setw(3) << ms 
            << "_" << dis(gen) << extension;
        
        m_path = directory / oss.str();
        attempts++;
    } while (std::filesystem::exists(m_path) && attempts < max_attempts);
    
    if (attempts >= max_attempts) {
        throw std::runtime_error("Failed to generate unique temporary filename");
    }
    
    // Create empty file
    std::ofstream ofs(m_path);
    if (!ofs) {
        throw std::runtime_error("Failed to create temporary file: " + m_path.string());
    }
    ofs.close();
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "Created scoped temporary file: ", m_path.string());
}

TempFile::~TempFile() {
    if (m_should_cleanup && std::filesystem::exists(m_path)) {
        try {
            std::filesystem::remove(m_path);
            Logger::getInstance().log(LogLevel::DEBUG,
                "Cleaned up scoped temporary file: ", m_path.string());
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR,
                "Failed to cleanup temporary file ", m_path.string(), ": ", e.what());
        }
    }
}

TempFile::TempFile(TempFile&& other) noexcept
    : m_path(std::move(other.m_path))
    , m_should_cleanup(other.m_should_cleanup) {
    other.m_should_cleanup = false;
}

TempFile& TempFile::operator=(TempFile&& other) noexcept {
    if (this != &other) {
        // Clean up current file if needed
        if (m_should_cleanup && std::filesystem::exists(m_path)) {
            std::filesystem::remove(m_path);
        }
        
        m_path = std::move(other.m_path);
        m_should_cleanup = other.m_should_cleanup;
        other.m_should_cleanup = false;
    }
    return *this;
}

void TempFile::remove() {
    if (std::filesystem::exists(m_path)) {
        std::filesystem::remove(m_path);
        Logger::getInstance().log(LogLevel::DEBUG,
            "Manually removed temporary file: ", m_path.string());
    }
    m_should_cleanup = false; // Skip cleanup in destructor
}

//=============================================================================
// TempDirectory implementation
//=============================================================================

TempDirectory::TempDirectory(const std::filesystem::path& parent_directory,
                            const std::string& prefix) {
    // Ensure parent directory exists
    std::filesystem::create_directories(parent_directory);
    
    int attempts = 0;
    const int max_attempts = 100;
    
    do {
        // Generate unique directory name
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        std::ostringstream oss;
        oss << prefix << time_t << "_" << std::setfill('0') << std::setw(3) << ms 
            << "_" << dis(gen);
        
        m_path = parent_directory / oss.str();
        attempts++;
    } while (std::filesystem::exists(m_path) && attempts < max_attempts);
    
    if (attempts >= max_attempts) {
        throw std::runtime_error("Failed to generate unique temporary directory name");
    }
    
    // Create directory
    std::filesystem::create_directories(m_path);
    
    Logger::getInstance().log(LogLevel::DEBUG,
        "Created scoped temporary directory: ", m_path.string());
}

TempDirectory::~TempDirectory() {
    if (m_should_cleanup && std::filesystem::exists(m_path)) {
        try {
            std::filesystem::remove_all(m_path);
            Logger::getInstance().log(LogLevel::DEBUG,
                "Cleaned up scoped temporary directory: ", m_path.string());
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR,
                "Failed to cleanup temporary directory ", m_path.string(), ": ", e.what());
        }
    }
}

TempDirectory::TempDirectory(TempDirectory&& other) noexcept
    : m_path(std::move(other.m_path))
    , m_should_cleanup(other.m_should_cleanup) {
    other.m_should_cleanup = false;
}

TempDirectory& TempDirectory::operator=(TempDirectory&& other) noexcept {
    if (this != &other) {
        // Clean up current directory if needed
        if (m_should_cleanup && std::filesystem::exists(m_path)) {
            std::filesystem::remove_all(m_path);
        }
        
        m_path = std::move(other.m_path);
        m_should_cleanup = other.m_should_cleanup;
        other.m_should_cleanup = false;
    }
    return *this;
}

void TempDirectory::remove() {
    if (std::filesystem::exists(m_path)) {
        std::filesystem::remove_all(m_path);
        Logger::getInstance().log(LogLevel::DEBUG,
            "Manually removed temporary directory: ", m_path.string());
    }
    m_should_cleanup = false; // Skip cleanup in destructor
}

} // namespace cql
