// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "types.hpp"
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <functional>

/**
 * @file intelligent_cache.hpp
 * @brief High-performance caching system for meta-prompt compilation results
 * 
 * This file provides the IntelligentCache system that enables sub-50ms
 * CACHED_LLM compilation by storing optimized prompts with semantic hashing.
 */

namespace cql {
namespace meta_prompt {

/**
 * @brief Cache entry containing compiled result and metadata
 */
struct CacheEntry {
    CompilationResult result;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_accessed;
    size_t access_count = 1;
    std::string cache_key;
    size_t insertion_sequence = 0;  // Track insertion order for LRU
    
    /**
     * @brief Check if entry has expired based on TTL
     */
    [[nodiscard]] bool is_expired(std::chrono::seconds ttl) const {
        auto now = std::chrono::system_clock::now();
        return (now - created_at) > ttl;
    }
    
    /**
     * @brief Update access statistics
     */
    void update_access() {
        last_accessed = std::chrono::system_clock::now();
        access_count++;
    }
};

/**
 * @brief Semantic hash key combining query content and compilation flags
 */
struct SemanticHashKey {
    std::string query_hash;
    std::string flags_hash;
    std::string combined_hash;
    
    SemanticHashKey(std::string_view query, const CompilerFlags& flags);
    
    // Constructor for reconstructing key from combined_hash (for import)
    explicit SemanticHashKey(const std::string& combined_hash_str);
    
    [[nodiscard]] bool operator==(const SemanticHashKey& other) const {
        return combined_hash == other.combined_hash;
    }
};

/**
 * @brief Hash function for SemanticHashKey
 */
struct SemanticHashKeyHasher {
    [[nodiscard]] size_t operator()(const SemanticHashKey& key) const {
        return std::hash<std::string>{}(key.combined_hash);
    }
};

/**
 * @brief Cache eviction policies for memory management
 */
enum class EvictionPolicy {
    LRU,        ///< Least Recently Used
    LFU,        ///< Least Frequently Used  
    TTL_BASED,  ///< Time To Live based
    MIXED       ///< Combination of LRU + TTL
};

/**
 * @brief Configuration for cache behavior
 */
struct CacheConfig {
    size_t max_entries = 1000;
    size_t max_memory_mb = 100;
    std::chrono::seconds ttl{3600}; // 1 hour
    EvictionPolicy eviction_policy = EvictionPolicy::MIXED;
    bool enable_compression = false;
    double eviction_threshold = 0.8; // Start eviction at 80% capacity
};

/**
 * @brief High-performance intelligent cache for compilation results
 * 
 * The IntelligentCache provides semantic-aware caching of meta-prompt
 * compilation results with automatic eviction, TTL management, and
 * comprehensive statistics tracking.
 * 
 * Key Features:
 * - Semantic hashing considering both query and compilation flags
 * - Multiple eviction policies (LRU, LFU, TTL, Mixed)
 * - Thread-safe operations with minimal lock contention
 * - Comprehensive metrics and performance monitoring
 * - Memory-efficient storage with optional compression
 * 
 * Performance Targets:
 * - Cache lookup: < 1ms
 * - Cache store: < 2ms
 * - Memory usage: < 100MB by default
 * - Hit rate: > 80% for repeated queries
 */
class IntelligentCache {
public:
    /**
     * @brief Create cache with default configuration
     */
    IntelligentCache();
    
    /**
     * @brief Create cache with custom configuration
     */
    explicit IntelligentCache(const CacheConfig& config);
    
    /**
     * @brief Destructor ensures proper cleanup
     */
    ~IntelligentCache();
    
    /**
     * @brief Attempt to retrieve cached compilation result
     * 
     * @param query The original query to compile
     * @param flags Compilation flags used for semantic hashing
     * @return Optional compilation result if cache hit, nullopt if miss
     */
    [[nodiscard]] std::optional<CompilationResult> get(
        std::string_view query, const CompilerFlags& flags);
    
    /**
     * @brief Store compilation result in cache
     * 
     * @param query The original query
     * @param flags Compilation flags
     * @param result The compilation result to cache
     * @return True if successfully stored, false if rejected
     */
    [[nodiscard]] bool put(std::string_view query, 
                          const CompilerFlags& flags,
                          const CompilationResult& result);
    
    /**
     * @brief Check if result exists in cache without retrieving
     * 
     * @param query The query to check
     * @param flags Compilation flags
     * @return True if cache contains valid entry
     */
    [[nodiscard]] bool contains(std::string_view query, 
                               const CompilerFlags& flags) const;
    
    /**
     * @brief Remove specific entry from cache
     * 
     * @param query The query to invalidate
     * @param flags Compilation flags
     * @return True if entry was removed
     */
    [[nodiscard]] bool invalidate(std::string_view query, 
                                 const CompilerFlags& flags);
    
    /**
     * @brief Clear all cached entries
     */
    void clear();
    
    /**
     * @brief Get current cache statistics
     * 
     * @return Complete statistics including hit rates and memory usage
     */
    [[nodiscard]] CacheStatistics get_statistics() const;
    
    /**
     * @brief Get current cache configuration
     */
    [[nodiscard]] const CacheConfig& get_config() const { return m_config; }
    
    /**
     * @brief Update cache configuration (thread-safe)
     * 
     * @param config New configuration to apply
     */
    void update_config(const CacheConfig& config);
    
    /**
     * @brief Trigger manual cleanup of expired entries
     * 
     * @return Number of entries removed
     */
    [[nodiscard]] size_t cleanup_expired();
    
    /**
     * @brief Get memory usage estimate in bytes
     */
    [[nodiscard]] size_t get_memory_usage() const;
    
    /**
     * @brief Export cache contents for analysis
     * 
     * @return Vector of all cache entries (expensive operation)
     */
    [[nodiscard]] std::vector<CacheEntry> export_entries() const;
    
    /**
     * @brief Warm cache with pre-computed results
     * 
     * @param entries Pre-computed cache entries
     * @return Number of entries successfully imported
     */
    [[nodiscard]] size_t import_entries(const std::vector<CacheEntry>& entries);

private:
    /**
     * @brief Perform eviction when cache reaches capacity
     */
    void perform_eviction();
    
    /**
     * @brief Select entries for eviction based on policy
     */
    [[nodiscard]] std::vector<SemanticHashKey> select_eviction_candidates();
    
    /**
     * @brief Calculate LRU score for entry
     */
    [[nodiscard]] double calculate_lru_score(const CacheEntry& entry) const;
    
    /**
     * @brief Calculate LFU score for entry
     */
    [[nodiscard]] double calculate_lfu_score(const CacheEntry& entry) const;
    
    /**
     * @brief Estimate memory usage of entry
     */
    [[nodiscard]] size_t estimate_entry_size(const CacheEntry& entry) const;
    
    /**
     * @brief Update statistics after cache operation
     */
    void update_statistics(bool cache_hit, size_t entry_size = 0);
    
    // Configuration
    CacheConfig m_config;
    
    // Cache storage
    std::unordered_map<SemanticHashKey, CacheEntry, SemanticHashKeyHasher> m_cache;
    
    // Thread safety
    mutable std::mutex m_cache_mutex;
    mutable std::mutex m_stats_mutex;
    
    // Statistics (protected by stats_mutex)
    mutable CacheStatistics m_stats;
    mutable size_t m_estimated_memory_usage = 0;
    
    // Insertion sequence tracking (protected by cache_mutex)
    size_t m_insertion_sequence = 0;
    
    // Background cleanup
    std::chrono::system_clock::time_point m_last_cleanup;
    static constexpr std::chrono::minutes CLEANUP_INTERVAL{5};
};

/**
 * @brief Factory function for creating intelligent cache
 * 
 * @param config Optional configuration, uses defaults if not provided
 * @return Unique pointer to configured cache instance
 */
[[nodiscard]] std::unique_ptr<IntelligentCache> create_intelligent_cache(
    const CacheConfig& config = {});

/**
 * @brief Utility functions for cache key generation
 */
namespace cache_utils {

/**
 * @brief Generate semantic hash for query content
 * 
 * Creates a hash that captures the semantic meaning of the query,
 * normalizing whitespace and extracting key semantic elements.
 * 
 * @param query The query to hash
 * @return Semantic hash string
 */
[[nodiscard]] std::string generate_semantic_hash(std::string_view query);

/**
 * @brief Generate hash for compilation flags
 * 
 * @param flags Compilation flags to hash
 * @return Flags hash string
 */
[[nodiscard]] std::string generate_flags_hash(const CompilerFlags& flags);

/**
 * @brief Normalize query for consistent hashing
 * 
 * Removes formatting differences while preserving semantic content.
 * 
 * @param query Query to normalize
 * @return Normalized query string
 */
[[nodiscard]] std::string normalize_query(std::string_view query);

} // namespace cache_utils

} // namespace meta_prompt
} // namespace cql
