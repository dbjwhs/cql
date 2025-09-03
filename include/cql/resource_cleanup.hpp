// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <filesystem>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_set>

namespace cql {

/**
 * @brief Resource cleanup manager for temporary files and directories
 * 
 * Provides RAII-style cleanup of temporary resources with automatic
 * cleanup on destruction and manual cleanup capabilities.
 */
class ResourceCleanup {
public:
    /**
     * @brief Cleanup function type
     */
    using CleanupFunction = std::function<void()>;

    ResourceCleanup() = default;
    ~ResourceCleanup();

    // Non-copyable but movable
    ResourceCleanup(const ResourceCleanup&) = delete;
    ResourceCleanup& operator=(const ResourceCleanup&) = delete;
    ResourceCleanup(ResourceCleanup&&) = default;
    ResourceCleanup& operator=(ResourceCleanup&&) = default;

    /**
     * @brief Register a temporary file for cleanup
     * @param filepath Path to file that should be cleaned up
     */
    void register_temp_file(const std::filesystem::path& filepath);

    /**
     * @brief Register a temporary directory for cleanup
     * @param dirpath Path to directory that should be cleaned up
     * @param recursive Whether to remove directory contents recursively
     */
    void register_temp_directory(const std::filesystem::path& dirpath, bool recursive = true);

    /**
     * @brief Register a custom cleanup function
     * @param cleanup_func Function to call during cleanup
     */
    void register_cleanup_function(CleanupFunction cleanup_func);

    /**
     * @brief Manually trigger cleanup of all registered resources
     * @return Number of items successfully cleaned up
     */
    std::size_t cleanup_now();

    /**
     * @brief Clear all registered cleanup items without executing them
     */
    void clear_registry();

    /**
     * @brief Enable or disable automatic cleanup on destruction
     * @param enabled Whether to cleanup automatically (default: true)
     */
    void set_auto_cleanup(bool enabled) { m_auto_cleanup = enabled; }

    /**
     * @brief Check if auto cleanup is enabled
     */
    [[nodiscard]] bool is_auto_cleanup_enabled() const { return m_auto_cleanup; }

    /**
     * @brief Get number of registered cleanup items
     */
    [[nodiscard]] std::size_t registered_count() const;

    /**
     * @brief Create a scoped temp file that will be automatically cleaned up
     * @param directory Directory to create temp file in (default: system temp)
     * @param prefix Filename prefix
     * @param extension File extension (default: ".tmp")
     * @return Path to created temporary file
     */
    [[nodiscard]] std::filesystem::path create_temp_file(
        const std::filesystem::path& directory = std::filesystem::temp_directory_path(),
        const std::string& prefix = "cql_temp_",
        const std::string& extension = ".tmp");

    /**
     * @brief Create a scoped temp directory that will be automatically cleaned up
     * @param parent_directory Parent directory (default: system temp)
     * @param prefix Directory name prefix
     * @return Path to created temporary directory
     */
    [[nodiscard]] std::filesystem::path create_temp_directory(
        const std::filesystem::path& parent_directory = std::filesystem::temp_directory_path(),
        const std::string& prefix = "cql_temp_");

private:
    struct CleanupItem {
        enum Type { FILE_TYPE, DIRECTORY_TYPE, FUNCTION_TYPE };
        Type type;
        std::filesystem::path path;
        bool recursive = false;
        CleanupFunction function;
        
        CleanupItem(const std::filesystem::path& p, Type t, bool rec = false)
            : type(t), path(p), recursive(rec) {}
        CleanupItem(CleanupFunction f) : type(FUNCTION_TYPE), function(std::move(f)) {}
    };

    std::vector<CleanupItem> m_cleanup_items;
    std::unordered_set<std::string> m_registered_paths; // For duplicate prevention
    bool m_auto_cleanup = true;

    /**
     * @brief Generate unique temporary filename
     */
    [[nodiscard]] std::string generate_unique_name(const std::string& prefix) const;
};

/**
 * @brief RAII wrapper for temporary file management
 * 
 * Automatically creates a temporary file and ensures it's cleaned up
 * when the object goes out of scope.
 */
class TempFile {
public:
    /**
     * @brief Create temporary file
     * @param directory Directory to create file in (default: system temp)
     * @param prefix Filename prefix  
     * @param extension File extension (default: ".tmp")
     */
    explicit TempFile(
        const std::filesystem::path& directory = std::filesystem::temp_directory_path(),
        const std::string& prefix = "cql_temp_",
        const std::string& extension = ".tmp");
    
    ~TempFile();

    // Non-copyable but movable
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&& other) noexcept;
    TempFile& operator=(TempFile&& other) noexcept;

    /**
     * @brief Get path to temporary file
     */
    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

    /**
     * @brief Check if file exists
     */
    [[nodiscard]] bool exists() const { return std::filesystem::exists(m_path); }

    /**
     * @brief Manually remove the file (cleanup will be skipped in destructor)
     */
    void remove();

private:
    std::filesystem::path m_path;
    bool m_should_cleanup = true;
};

/**
 * @brief RAII wrapper for temporary directory management
 * 
 * Automatically creates a temporary directory and ensures it's cleaned up
 * when the object goes out of scope.
 */
class TempDirectory {
public:
    /**
     * @brief Create temporary directory
     * @param parent_directory Parent directory (default: system temp)
     * @param prefix Directory name prefix
     */
    explicit TempDirectory(
        const std::filesystem::path& parent_directory = std::filesystem::temp_directory_path(),
        const std::string& prefix = "cql_temp_");
    
    ~TempDirectory();

    // Non-copyable but movable
    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
    TempDirectory(TempDirectory&& other) noexcept;
    TempDirectory& operator=(TempDirectory&& other) noexcept;

    /**
     * @brief Get path to temporary directory
     */
    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

    /**
     * @brief Check if directory exists
     */
    [[nodiscard]] bool exists() const { return std::filesystem::exists(m_path); }

    /**
     * @brief Manually remove the directory (cleanup will be skipped in destructor)
     */
    void remove();

private:
    std::filesystem::path m_path;
    bool m_should_cleanup = true;
};

} // namespace cql
