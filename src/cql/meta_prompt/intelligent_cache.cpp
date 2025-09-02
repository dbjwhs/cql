// MIT License
// Copyright (c) 2025 dbjwhs

#include "cql/meta_prompt/intelligent_cache.hpp"
#include "cql/project_utils.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <shared_mutex>

namespace cql {
namespace meta_prompt {

//=============================================================================
// SemanticHashKey implementation
//=============================================================================

SemanticHashKey::SemanticHashKey(std::string_view query, const CompilerFlags& flags) {
    query_hash = cache_utils::generate_semantic_hash(query);
    flags_hash = cache_utils::generate_flags_hash(flags);
    
    // Combine hashes for final key
    std::ostringstream oss;
    oss << query_hash << ":" << flags_hash;
    combined_hash = oss.str();
}

SemanticHashKey::SemanticHashKey(const std::string& combined_hash_str) 
    : combined_hash(combined_hash_str) {
    // Parse the combined hash to extract query_hash and flags_hash
    auto colon_pos = combined_hash.find(':');
    if (colon_pos != std::string::npos) {
        query_hash = combined_hash.substr(0, colon_pos);
        flags_hash = combined_hash.substr(colon_pos + 1);
    }
    // If parsing fails, we keep empty query_hash and flags_hash,
    // but the combined_hash is still valid for equality comparison
}

//=============================================================================
// IntelligentCache implementation  
//=============================================================================

IntelligentCache::IntelligentCache() : IntelligentCache(CacheConfig{}) {
}

IntelligentCache::IntelligentCache(const CacheConfig& config) 
    : m_config(config)
    , m_last_cleanup(std::chrono::system_clock::now()) {
    
    Logger::getInstance().log(LogLevel::INFO, 
        "IntelligentCache initialized with max_entries=", m_config.max_entries,
        ", max_memory=", m_config.max_memory_mb, "MB");
}

IntelligentCache::~IntelligentCache() {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    Logger::getInstance().log(LogLevel::INFO, 
        "IntelligentCache shutting down with ", m_cache.size(), " entries");
}

std::optional<CompilationResult> IntelligentCache::get(
    std::string_view query, const CompilerFlags& flags) {
    
    SemanticHashKey key(query, flags);
    
    // Try to find entry with read lock
    {
        std::lock_guard<std::mutex> lock(m_cache_mutex);
        
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            update_statistics(false);
            return std::nullopt;
        }
        
        // Check if entry has expired
        if (it->second.is_expired(m_config.ttl)) {
            // Entry expired, will remove it with write lock
        } else {
            // Cache hit - update access stats and return result
            // Note: We need to modify the entry, so we'll need write access
            update_statistics(true);
        }
    }
    
    // Get write lock to update access stats or remove expired entry
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        update_statistics(false);
        return std::nullopt;
    }
    
    // Check expiration again with write lock
    if (it->second.is_expired(m_config.ttl)) {
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Cache entry expired, removing: ", key.combined_hash);
        m_cache.erase(it);
        update_statistics(false);
        return std::nullopt;
    }
    
    // Update access statistics
    it->second.update_access();
    
    // Only log cache hits occasionally to avoid performance impact
    if (it->second.access_count % 100 == 1) {
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Cache hit for key: ", key.combined_hash, " (access #", it->second.access_count, ")");
    }
    
    // Mark result as cache hit
    auto result = it->second.result;
    result.metrics.cache_hit = true;
    result.metrics.compilation_time = std::chrono::milliseconds(1); // Cache retrieval time
    
    return result;
}

bool IntelligentCache::put(std::string_view query, 
                          const CompilerFlags& flags,
                          const CompilationResult& result) {
    
    // Only cache successful results
    if (!result.success) {
        return false;
    }
    
    SemanticHashKey key(query, flags);
    
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    // Check if we need eviction before adding new entry
    // If adding this entry would exceed max_entries, evict first
    if (m_cache.size() >= m_config.max_entries) {
        perform_eviction();
    }
    
    // Create cache entry
    CacheEntry entry;
    entry.result = result;
    entry.created_at = std::chrono::system_clock::now();
    entry.last_accessed = entry.created_at;
    entry.cache_key = key.combined_hash;
    entry.access_count = 1;
    entry.insertion_sequence = ++m_insertion_sequence;
    
    // Estimate memory usage
    size_t entry_size = estimate_entry_size(entry);
    
    // Check memory limits
    if (m_estimated_memory_usage + entry_size > m_config.max_memory_mb * 1024 * 1024) {
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Cache memory limit reached, performing eviction");
        perform_eviction();
        
        // Check again after eviction
        if (m_estimated_memory_usage + entry_size > m_config.max_memory_mb * 1024 * 1024) {
            Logger::getInstance().log(LogLevel::NORMAL, 
                "Cannot cache entry, would exceed memory limit");
            return false;
        }
    }
    
    // Insert or update entry
    auto [it, inserted] = m_cache.insert_or_assign(key, std::move(entry));
    
    if (inserted) {
        m_estimated_memory_usage += entry_size;
        update_statistics(false, entry_size);
        
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Cached new entry: ", key.combined_hash);
    } else {
        Logger::getInstance().log(LogLevel::DEBUG, 
            "Updated existing cache entry: ", key.combined_hash);
    }
    
    // Periodic cleanup check
    auto now = std::chrono::system_clock::now();
    if (now - m_last_cleanup > CLEANUP_INTERVAL) {
        size_t removed = cleanup_expired();
        if (removed > 0) {
            Logger::getInstance().log(LogLevel::DEBUG, 
                "Cleaned up ", removed, " expired entries");
        }
        m_last_cleanup = now;
    }
    
    return true;
}

bool IntelligentCache::contains(std::string_view query, 
                               const CompilerFlags& flags) const {
    SemanticHashKey key(query, flags);
    
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return false;
    }
    
    return !it->second.is_expired(m_config.ttl);
}

bool IntelligentCache::invalidate(std::string_view query, 
                                 const CompilerFlags& flags) {
    SemanticHashKey key(query, flags);
    
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return false;
    }
    
    size_t entry_size = estimate_entry_size(it->second);
    m_estimated_memory_usage -= entry_size;
    
    m_cache.erase(it);
    
    Logger::getInstance().log(LogLevel::DEBUG, 
        "Invalidated cache entry: ", key.combined_hash);
    
    return true;
}

void IntelligentCache::clear() {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    size_t count = m_cache.size();
    m_cache.clear();
    m_estimated_memory_usage = 0;
    
    // Reset statistics
    {
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        m_stats = CacheStatistics{};
    }
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Cache cleared, removed ", count, " entries");
}

CacheStatistics IntelligentCache::get_statistics() const {
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    
    // Update current state
    m_stats.entry_count = m_cache.size();
    m_stats.cache_size_bytes = m_estimated_memory_usage;
    m_stats.last_cleanup = m_last_cleanup;
    
    // Calculate hit rate
    if (m_stats.total_requests > 0) {
        m_stats.hit_rate = static_cast<double>(m_stats.cache_hits) / m_stats.total_requests;
    }
    
    return m_stats;
}

void IntelligentCache::update_config(const CacheConfig& config) {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    CacheConfig old_config = m_config;
    m_config = config;
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Cache configuration updated");
    
    // If max_entries decreased, might need eviction
    if (config.max_entries < old_config.max_entries && 
        m_cache.size() > config.max_entries * config.eviction_threshold) {
        perform_eviction();
    }
}

size_t IntelligentCache::cleanup_expired() {
    // This method assumes caller already has write lock
    
    size_t removed_count = 0;
    
    for (auto it = m_cache.begin(); it != m_cache.end();) {
        if (it->second.is_expired(m_config.ttl)) {
            size_t entry_size = estimate_entry_size(it->second);
            m_estimated_memory_usage -= entry_size;
            it = m_cache.erase(it);
            removed_count++;
        } else {
            ++it;
        }
    }
    
    return removed_count;
}

size_t IntelligentCache::get_memory_usage() const {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    return m_estimated_memory_usage;
}

std::vector<CacheEntry> IntelligentCache::export_entries() const {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    std::vector<CacheEntry> entries;
    entries.reserve(m_cache.size());
    
    for (const auto& [key, entry] : m_cache) {
        entries.push_back(entry);
    }
    
    return entries;
}

size_t IntelligentCache::import_entries(const std::vector<CacheEntry>& entries) {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    size_t imported_count = 0;
    
    for (const auto& entry : entries) {
        // Skip expired entries
        if (entry.is_expired(m_config.ttl)) {
            continue;
        }
        
        // Reconstruct key from stored cache_key
        SemanticHashKey key(entry.cache_key);
        
        size_t entry_size = estimate_entry_size(entry);
        
        // Check capacity limits
        if (m_cache.size() >= m_config.max_entries ||
            m_estimated_memory_usage + entry_size > m_config.max_memory_mb * 1024 * 1024) {
            break;
        }
        
        m_cache.insert_or_assign(key, entry);
        m_estimated_memory_usage += entry_size;
        imported_count++;
    }
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Imported ", imported_count, " cache entries");
    
    return imported_count;
}

void IntelligentCache::perform_eviction() {
    // This method assumes caller already has write lock
    
    if (m_cache.size() < m_config.max_entries) {
        return; // No eviction needed
    }
    
    auto candidates = select_eviction_candidates();
    
    size_t removed_count = 0;
    for (const auto& key : candidates) {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            size_t entry_size = estimate_entry_size(it->second);
            m_estimated_memory_usage -= entry_size;
            m_cache.erase(it);
            removed_count++;
            
            if (m_cache.size() <= m_config.max_entries) {
                break;
            }
        }
    }
    
    Logger::getInstance().log(LogLevel::DEBUG, 
        "Evicted ", removed_count, " cache entries");
}

std::vector<SemanticHashKey> IntelligentCache::select_eviction_candidates() {
    std::vector<std::pair<SemanticHashKey, double>> scored_entries;
    scored_entries.reserve(m_cache.size());
    
    for (const auto& [key, entry] : m_cache) {
        double score = 0.0;
        
        switch (m_config.eviction_policy) {
            case EvictionPolicy::LRU:
                score = calculate_lru_score(entry);
                break;
            case EvictionPolicy::LFU:
                score = calculate_lfu_score(entry);
                break;
            case EvictionPolicy::TTL_BASED:
                score = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now() - entry.created_at).count();
                break;
            case EvictionPolicy::MIXED:
                score = 0.6 * calculate_lru_score(entry) + 0.4 * calculate_lfu_score(entry);
                break;
        }
        
        scored_entries.emplace_back(key, score);
    }
    
    // Sort by score - different policies need different sorting
    if (m_config.eviction_policy == EvictionPolicy::LFU) {
        // LFU: Lower scores (fewer accesses) should be evicted first
        std::sort(scored_entries.begin(), scored_entries.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
    } else {
        // LRU, TTL_BASED, MIXED: Higher scores should be evicted first
        std::sort(scored_entries.begin(), scored_entries.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
    }
    
    std::vector<SemanticHashKey> candidates;
    // Calculate how many entries we need to evict to stay within max_entries
    size_t evict_count = (m_cache.size() >= m_config.max_entries) ? 
                        (m_cache.size() - m_config.max_entries + 1) : 0;
    
    
    for (size_t i = 0; i < std::min(evict_count, scored_entries.size()); ++i) {
        candidates.push_back(scored_entries[i].first);
    }
    
    return candidates;
}

double IntelligentCache::calculate_lru_score(const CacheEntry& entry) const {
    auto now = std::chrono::system_clock::now();
    
    // Calculate time since last access
    auto access_age = std::chrono::duration_cast<std::chrono::microseconds>(now - entry.last_accessed);
    
    // For LRU, we want older entries to have higher scores (more likely to evict)
    // Use insertion_sequence as tie-breaker when times are identical
    double time_score = static_cast<double>(access_age.count());
    
    // If last_accessed equals created_at (never accessed after creation),
    // fall back to insertion order - earlier entries get LOWER scores for eviction
    if (entry.last_accessed == entry.created_at) {
        // Lower sequence = inserted earlier = lower score = more likely to evict (correct for LRU)
        // Add time component for proper scaling, but sequence determines order
        time_score = time_score + static_cast<double>(entry.insertion_sequence);
    }
    
    return time_score;
}

double IntelligentCache::calculate_lfu_score(const CacheEntry& entry) const {
    // Lower access count = lower score = more likely to evict
    return static_cast<double>(entry.access_count);
}

size_t IntelligentCache::estimate_entry_size(const CacheEntry& entry) const {
    size_t size = 0;
    
    // Result data
    size += entry.result.compiled_prompt.size();
    size += entry.result.error_message.size();
    size += entry.result.original_query.size();
    
    // Cache metadata
    size += entry.cache_key.size();
    size += sizeof(CacheEntry) - sizeof(std::string) * 4; // Fixed-size fields
    
    return size;
}

void IntelligentCache::update_statistics(bool cache_hit, size_t entry_size) {
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    
    m_stats.total_requests++;
    
    if (cache_hit) {
        m_stats.cache_hits++;
    } else {
        m_stats.cache_misses++;
    }
    
    if (entry_size > 0) {
        // This is a new entry being added
        m_stats.entry_count++;
    }
}

//=============================================================================
// Factory function
//=============================================================================

std::unique_ptr<IntelligentCache> create_intelligent_cache(const CacheConfig& config) {
    return std::make_unique<IntelligentCache>(config);
}

//=============================================================================
// Cache utilities
//=============================================================================

namespace cache_utils {

std::string generate_semantic_hash(std::string_view query) {
    // Normalize the query first
    std::string normalized = normalize_query(query);
    
    // Simple hash implementation
    std::hash<std::string> hasher;
    size_t hash_value = hasher(normalized);
    
    // Convert to hex string
    std::ostringstream oss;
    oss << "q_" << std::hex << hash_value;
    return oss.str();
}

std::string generate_flags_hash(const CompilerFlags& flags) {
    std::ostringstream oss;
    oss << "f_" << static_cast<int>(flags.mode)
        << "_" << static_cast<int>(flags.goal)
        << "_" << (flags.validate_semantics ? "1" : "0")
        << "_" << (flags.enable_caching ? "1" : "0")
        << "_" << (flags.use_deterministic ? "1" : "0")
        << "_" << std::hash<std::string>{}(flags.domain)
        << "_" << std::fixed << std::setprecision(3) << flags.cost_budget
        << "_" << std::fixed << std::setprecision(2) << flags.temperature;
    
    // Hash the combined string for compactness
    std::hash<std::string> hasher;
    size_t hash_value = hasher(oss.str());
    
    std::ostringstream result;
    result << std::hex << hash_value;
    return result.str();
}

std::string normalize_query(std::string_view query) {
    std::string result;
    result.reserve(query.size());
    
    bool in_whitespace = true; // Start as true to skip leading whitespace
    bool in_quotes = false;
    char quote_char = '\0';
    
    for (char c : query) {
        if (!in_quotes && (c == '"' || c == '\'')) {
            in_quotes = true;
            quote_char = c;
            result += c;
            in_whitespace = false;
        } else if (in_quotes && c == quote_char) {
            in_quotes = false;
            quote_char = '\0';
            result += c;
            in_whitespace = false;
        } else if (!in_quotes && std::isspace(c)) {
            if (!in_whitespace && !result.empty()) { // Only add space if we have content
                result += ' ';
                in_whitespace = true;
            }
        } else {
            result += std::tolower(c);
            in_whitespace = false;
        }
    }
    
    // Trim trailing whitespace
    while (!result.empty() && std::isspace(result.back())) {
        result.pop_back();
    }
    
    return result;
}

} // namespace cache_utils

} // namespace meta_prompt
} // namespace cql
