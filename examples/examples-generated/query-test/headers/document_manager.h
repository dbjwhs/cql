// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "document_types.h"
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>

namespace doc_system {

// Singleton Pattern: Thread-safe DocumentManager
class DocumentManager {
public:
    // Delete copy and move operations to ensure singleton behavior
    DocumentManager(const DocumentManager&) = delete;
    DocumentManager& operator=(const DocumentManager&) = delete;
    DocumentManager(DocumentManager&&) = delete;
    DocumentManager& operator=(DocumentManager&&) = delete;
    
    // Get singleton instance
    static DocumentManager& getInstance() {
        static DocumentManager instance;  // C++11 guarantees thread-safe initialization
        return instance;
    }
    
    // Register a document in the manager
    void registerDocument(DocumentPtr document) {
        if (!document) {
            throw std::invalid_argument("Cannot register a null document");
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        const std::string& filename = document->getFilename();
        documents_[filename] = document;
    }
    
    // Retrieve a document by filename
    DocumentPtr getDocument(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = documents_.find(filename);
        if (it != documents_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Remove a document from the manager
    bool unregisterDocument(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        return documents_.erase(filename) > 0;
    }
    
    // Get all registered documents
    std::vector<DocumentPtr> getAllDocuments() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<DocumentPtr> result;
        result.reserve(documents_.size());
        
        for (const auto& [_, doc] : documents_) {
            result.push_back(doc);
        }
        
        return result;
    }
    
    // Get count of registered documents
    size_t getDocumentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return documents_.size();
    }

private:
    // Private constructor for singleton
    DocumentManager() = default;
    
    // Thread synchronization
    mutable std::mutex mutex_;
    
    // Document storage
    std::unordered_map<std::string, DocumentPtr> documents_;
};

} // namespace doc_system
