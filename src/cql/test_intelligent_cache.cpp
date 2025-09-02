// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "cql/meta_prompt/intelligent_cache.hpp"
#include "cql/logger_manager.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <random>

/**
 * @file test_intelligent_cache.cpp
 * @brief Comprehensive tests for IntelligentCache system
 * 
 * Tests the caching system that enables CACHED_LLM mode with <50ms performance.
 */

namespace cql {
namespace meta_prompt {

class IntelligentCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration for most tests
        config = CacheConfig{
            .max_entries = 100,
            .max_memory_mb = 10,
            .ttl = std::chrono::seconds(3600),
            .eviction_policy = EvictionPolicy::MIXED
        };
        
        cache = create_intelligent_cache(config);
        
        // Create sample data
        sample_query = "@description \"Create a test function\"";
        sample_flags = CompilerFlags{
            .mode = CompilationMode::LOCAL_ONLY,
            .goal = OptimizationGoal::BALANCED
        };
        
        sample_result = create_sample_result();
    }
    
    CompilationResult create_sample_result() {
        CompilationMetrics metrics;
        metrics.compilation_time = std::chrono::milliseconds(100);
        metrics.used_llm = false;
        metrics.input_tokens = 50;
        metrics.output_tokens = 40;
        metrics.token_reduction_percent = 20.0f;
        
        ValidationResult validation;
        validation.is_semantically_equivalent = true;
        validation.confidence_score = 0.95;
        
        auto result = CompilationResult::success_result(
            "Optimized test function query", metrics, validation);
        result.original_query = sample_query;
        result.flags_used = sample_flags;
        
        return result;
    }
    
    CacheConfig config;
    std::unique_ptr<IntelligentCache> cache;
    std::string sample_query;
    CompilerFlags sample_flags;
    CompilationResult sample_result;
};

// Test basic cache operations
TEST_F(IntelligentCacheTest, BasicCacheOperations) {
    // Initially empty
    EXPECT_FALSE(cache->contains(sample_query, sample_flags));
    
    auto miss_result = cache->get(sample_query, sample_flags);
    EXPECT_FALSE(miss_result.has_value());
    
    // Store result
    EXPECT_TRUE(cache->put(sample_query, sample_flags, sample_result));
    
    // Should now contain the entry
    EXPECT_TRUE(cache->contains(sample_query, sample_flags));
    
    // Retrieve result
    auto hit_result = cache->get(sample_query, sample_flags);
    ASSERT_TRUE(hit_result.has_value());
    EXPECT_EQ(hit_result->compiled_prompt, sample_result.compiled_prompt);
    EXPECT_TRUE(hit_result->metrics.cache_hit);
    EXPECT_EQ(hit_result->metrics.compilation_time, std::chrono::milliseconds(1));
}

// Test cache statistics
TEST_F(IntelligentCacheTest, CacheStatistics) {
    auto initial_stats = cache->get_statistics();
    EXPECT_EQ(initial_stats.total_requests, 0);
    EXPECT_EQ(initial_stats.cache_hits, 0);
    EXPECT_EQ(initial_stats.cache_misses, 0);
    
    // Generate some cache misses
    for (int i = 0; i < 5; ++i) {
        [[maybe_unused]] auto result = cache->get(sample_query + std::to_string(i), sample_flags);
    }
    
    // Store some entries
    for (int i = 0; i < 3; ++i) {
        [[maybe_unused]] auto success = cache->put(sample_query + std::to_string(i), sample_flags, sample_result);
    }
    
    // Generate some cache hits
    for (int i = 0; i < 3; ++i) {
        [[maybe_unused]] auto result = cache->get(sample_query + std::to_string(i), sample_flags);
    }
    
    auto final_stats = cache->get_statistics();
    EXPECT_EQ(final_stats.total_requests, 11); // 5 misses + 3 stores + 3 hits
    EXPECT_EQ(final_stats.cache_hits, 3);
    EXPECT_EQ(final_stats.cache_misses, 8); // 5 initial misses + 3 during puts
    EXPECT_DOUBLE_EQ(final_stats.hit_rate, 3.0 / 11.0);
    EXPECT_EQ(final_stats.entry_count, 3);
}

// Test semantic hashing
TEST_F(IntelligentCacheTest, SemanticHashing) {
    // Queries with different formatting but same semantic meaning
    std::string query1 = "@description \"Create a function\"";
    std::string query2 = "  @description   \"Create a function\"  ";
    std::string query3 = "@DESCRIPTION \"create a function\"";
    
    // Store with first formatting
    [[maybe_unused]] auto success = cache->put(query1, sample_flags, sample_result);
    
    // Should find with other formatting (semantic equivalence)
    EXPECT_TRUE(cache->contains(query2, sample_flags));
    EXPECT_TRUE(cache->contains(query3, sample_flags));
    
    // Different semantic content should miss
    std::string different_query = "@description \"Create a class\"";
    EXPECT_FALSE(cache->contains(different_query, sample_flags));
}

// Test different compilation flags create different cache entries
TEST_F(IntelligentCacheTest, FlagsAffectCaching) {
    CompilerFlags flags1{.mode = CompilationMode::LOCAL_ONLY, .goal = OptimizationGoal::BALANCED};
    CompilerFlags flags2{.mode = CompilationMode::LOCAL_ONLY, .goal = OptimizationGoal::REDUCE_TOKENS};
    
    // Store with first flags
    [[maybe_unused]] auto success1 = cache->put(sample_query, flags1, sample_result);
    
    // Should miss with different flags
    EXPECT_FALSE(cache->contains(sample_query, flags2));
    
    // Store with second flags
    [[maybe_unused]] auto success2 = cache->put(sample_query, flags2, sample_result);
    
    // Should have both entries
    EXPECT_TRUE(cache->contains(sample_query, flags1));
    EXPECT_TRUE(cache->contains(sample_query, flags2));
}

// Test cache eviction
TEST_F(IntelligentCacheTest, CacheEviction) {
    // Create small cache for testing eviction
    CacheConfig small_config{
        .max_entries = 5,
        .max_memory_mb = 1,
        .eviction_policy = EvictionPolicy::LRU,
        .eviction_threshold = 0.8
    };
    auto small_cache = create_intelligent_cache(small_config);
    
    // Fill cache beyond eviction threshold
    for (int i = 0; i < 6; ++i) {
        std::string query = sample_query + std::to_string(i);
        [[maybe_unused]] auto success = small_cache->put(query, sample_flags, sample_result);
    }
    
    auto stats = small_cache->get_statistics();
    EXPECT_LE(stats.entry_count, 5); // Should have evicted entries
    
    // Oldest entries should be evicted (LRU policy)
    EXPECT_FALSE(small_cache->contains(sample_query + "0", sample_flags));
    EXPECT_TRUE(small_cache->contains(sample_query + "5", sample_flags));
}

// Test TTL expiration
TEST_F(IntelligentCacheTest, TTLExpiration) {
    // Create cache with very short TTL
    CacheConfig short_ttl_config{
        .max_entries = 100,
        .ttl = std::chrono::seconds(1)
    };
    auto ttl_cache = create_intelligent_cache(short_ttl_config);
    
    // Store entry
    [[maybe_unused]] auto success = ttl_cache->put(sample_query, sample_flags, sample_result);
    EXPECT_TRUE(ttl_cache->contains(sample_query, sample_flags));
    
    // Wait for expiration  
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Entry should be expired
    EXPECT_FALSE(ttl_cache->contains(sample_query, sample_flags));
    
    // Trying to get should return nullopt and remove expired entry
    auto result = ttl_cache->get(sample_query, sample_flags);
    EXPECT_FALSE(result.has_value());
}

// Test manual cleanup
TEST_F(IntelligentCacheTest, ManualCleanup) {
    CacheConfig short_ttl_config{
        .ttl = std::chrono::seconds(1)
    };
    auto ttl_cache = create_intelligent_cache(short_ttl_config);
    
    // Add several entries
    for (int i = 0; i < 5; ++i) {
        [[maybe_unused]] auto success = ttl_cache->put(sample_query + std::to_string(i), sample_flags, sample_result);
    }
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Manual cleanup should remove expired entries
    size_t removed = ttl_cache->cleanup_expired();
    EXPECT_EQ(removed, 5);
    
    auto stats = ttl_cache->get_statistics();
    EXPECT_EQ(stats.entry_count, 0);
}

// Test cache invalidation
TEST_F(IntelligentCacheTest, CacheInvalidation) {
    // Store entry
    [[maybe_unused]] auto success = cache->put(sample_query, sample_flags, sample_result);
    EXPECT_TRUE(cache->contains(sample_query, sample_flags));
    
    // Invalidate entry
    EXPECT_TRUE(cache->invalidate(sample_query, sample_flags));
    EXPECT_FALSE(cache->contains(sample_query, sample_flags));
    
    // Invalidating non-existent entry should return false
    EXPECT_FALSE(cache->invalidate("non-existent", sample_flags));
}

// Test cache clear
TEST_F(IntelligentCacheTest, CacheClear) {
    // Add multiple entries
    for (int i = 0; i < 10; ++i) {
        [[maybe_unused]] auto success = cache->put(sample_query + std::to_string(i), sample_flags, sample_result);
    }
    
    auto stats_before = cache->get_statistics();
    EXPECT_GT(stats_before.entry_count, 0);
    
    // Clear cache
    cache->clear();
    
    auto stats_after = cache->get_statistics();
    EXPECT_EQ(stats_after.entry_count, 0);
    EXPECT_EQ(stats_after.cache_hits, 0);
    EXPECT_EQ(stats_after.cache_misses, 0);
    EXPECT_EQ(stats_after.total_requests, 0);
}

// Test memory usage estimation
TEST_F(IntelligentCacheTest, MemoryUsageEstimation) {
    auto initial_usage = cache->get_memory_usage();
    EXPECT_EQ(initial_usage, 0);
    
    // Add entry
    [[maybe_unused]] auto success = cache->put(sample_query, sample_flags, sample_result);
    
    auto usage_after_put = cache->get_memory_usage();
    EXPECT_GT(usage_after_put, 0);
    
    // Memory usage should be reasonable (not excessive)
    EXPECT_LT(usage_after_put, 10000); // Less than 10KB for one entry
}

// Test different eviction policies
TEST_F(IntelligentCacheTest, EvictionPolicies) {
    // Test LFU eviction
    CacheConfig lfu_config{
        .max_entries = 3,
        .eviction_policy = EvictionPolicy::LFU,
        .eviction_threshold = 0.8
    };
    auto lfu_cache = create_intelligent_cache(lfu_config);
    
    // Add entries
    [[maybe_unused]] auto success1 = lfu_cache->put("query1", sample_flags, sample_result);
    [[maybe_unused]] auto success2 = lfu_cache->put("query2", sample_flags, sample_result);
    [[maybe_unused]] auto success3 = lfu_cache->put("query3", sample_flags, sample_result);
    
    // Access first entry multiple times to increase frequency
    for (int i = 0; i < 5; ++i) {
        [[maybe_unused]] auto result = lfu_cache->get("query1", sample_flags);
    }
    
    // Add fourth entry, should evict least frequently used
    [[maybe_unused]] auto success4 = lfu_cache->put("query4", sample_flags, sample_result);
    
    // Most frequently accessed should still be there
    EXPECT_TRUE(lfu_cache->contains("query1", sample_flags));
    EXPECT_TRUE(lfu_cache->contains("query4", sample_flags));
}

// Test configuration updates
TEST_F(IntelligentCacheTest, ConfigurationUpdates) {
    // Store some entries
    for (int i = 0; i < 5; ++i) {
        [[maybe_unused]] auto success = cache->put(sample_query + std::to_string(i), sample_flags, sample_result);
    }
    
    // Update configuration with smaller max_entries
    CacheConfig new_config{
        .max_entries = 3,
        .eviction_threshold = 0.8
    };
    cache->update_config(new_config);
    
    // Should have evicted entries
    auto stats = cache->get_statistics();
    EXPECT_LE(stats.entry_count, 3);
}

// Test export/import functionality
TEST_F(IntelligentCacheTest, ExportImport) {
    // Add several entries
    std::vector<std::string> queries;
    for (int i = 0; i < 5; ++i) {
        std::string query = sample_query + std::to_string(i);
        queries.push_back(query);
        [[maybe_unused]] auto success = cache->put(query, sample_flags, sample_result);
    }
    
    // Export entries
    auto exported = cache->export_entries();
    EXPECT_EQ(exported.size(), 5);
    
    // Clear and import to new cache
    auto new_cache = create_intelligent_cache(config);
    size_t imported = new_cache->import_entries(exported);
    EXPECT_EQ(imported, 5);
    
    // Verify all entries are accessible
    for (const auto& query : queries) {
        EXPECT_TRUE(new_cache->contains(query, sample_flags));
    }
}

// Test thread safety
TEST_F(IntelligentCacheTest, ThreadSafety) {
    const int num_threads = 4;
    std::vector<std::thread> threads;
    
    // Launch threads doing concurrent operations
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < 100; ++i) { // operations_per_thread is 100
                std::string query = sample_query + "_t" + std::to_string(t) + "_" + std::to_string(i);
                
                // Mix of operations
                if (i % 3 == 0) {
                    [[maybe_unused]] auto success = cache->put(query, sample_flags, sample_result);
                } else if (i % 3 == 1) {
                    [[maybe_unused]] auto result = cache->get(query, sample_flags);
                } else {
                    [[maybe_unused]] auto contains = cache->contains(query, sample_flags);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should not crash and should have reasonable statistics
    auto stats = cache->get_statistics();
    EXPECT_GT(stats.total_requests, 0);
}

// Test performance requirements
TEST_F(IntelligentCacheTest, PerformanceRequirements) {
    // Pre-populate cache
    [[maybe_unused]] auto success = cache->put(sample_query, sample_flags, sample_result);
    
    // Measure cache hit performance
    auto start = std::chrono::high_resolution_clock::now();
    
    constexpr int iterations = 1000;
    for (int i = 0; i < iterations; ++i) {
        auto result = cache->get(sample_query, sample_flags);
        EXPECT_TRUE(result.has_value());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should average < 50 microseconds per lookup (much faster than 1ms requirement)
    double avg_microseconds = static_cast<double>(duration.count()) / iterations;
    EXPECT_LT(avg_microseconds, 50.0);
    
    LoggerManager::log(LogLevel::INFO, 
        "Cache lookup performance: " + std::to_string(avg_microseconds) + " μs average");
}

// Test cache utility functions
TEST_F(IntelligentCacheTest, CacheUtilities) {
    // Test query normalization
    std::string query1 = "  @description   \"test\"  ";
    std::string query2 = "@DESCRIPTION \"Test\"";
    
    auto normalized1 = cache_utils::normalize_query(query1);
    auto normalized2 = cache_utils::normalize_query(query2);
    
    // Should normalize to same result
    EXPECT_EQ(normalized1, normalized2);
    
    // Test semantic hashing consistency
    auto hash1 = cache_utils::generate_semantic_hash(query1);
    auto hash2 = cache_utils::generate_semantic_hash(query2);
    EXPECT_EQ(hash1, hash2);
    
    // Test flags hashing
    auto flags_hash1 = cache_utils::generate_flags_hash(sample_flags);
    auto flags_hash2 = cache_utils::generate_flags_hash(sample_flags);
    EXPECT_EQ(flags_hash1, flags_hash2);
    
    // Different flags should produce different hashes
    CompilerFlags different_flags = sample_flags;
    different_flags.goal = OptimizationGoal::REDUCE_TOKENS;
    auto different_hash = cache_utils::generate_flags_hash(different_flags);
    EXPECT_NE(flags_hash1, different_hash);
}

// Test error conditions
TEST_F(IntelligentCacheTest, ErrorConditions) {
    // Cannot cache failed results
    CompilationResult failed_result = CompilationResult::error_result("Test error");
    EXPECT_FALSE(cache->put(sample_query, sample_flags, failed_result));
    
    // Cache should handle very large queries gracefully
    std::string huge_query(10000, 'x'); // 10KB query
    EXPECT_TRUE(cache->put(huge_query, sample_flags, sample_result));
}

} // namespace meta_prompt
} // namespace cql
