// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <variant>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <type_traits>
#include <concepts>
#include <format>

namespace doc_system {

// Forward declarations
class Document;
class DocumentObserver;
class MessageQueue;

// Type definitions for improved readability
using DocumentPtr = std::shared_ptr<Document>;
using ObserverPtr = std::shared_ptr<DocumentObserver>;
using MetadataMap = std::unordered_map<std::string, std::string>;

// Enum for document types
enum class DocumentType {
    PDF,
    Word,
    Text,
    Unknown
};

// Document interface - base class for all document types
class Document {
public:
    virtual ~Document() = default;
    
    // Core document operations
    virtual DocumentType getType() const = 0;
    virtual std::string getFilename() const = 0;
    virtual void save() = 0;
    virtual void load() = 0;
    
    // Document content management
    virtual void setHeader(const std::string& header) = 0;
    virtual void setContent(const std::string& content) = 0;
    virtual void setFooter(const std::string& footer) = 0;
    virtual void setMetadata(const MetadataMap& metadata) = 0;
    virtual void updateContent(const std::string& newContent) = 0;
    
    // Observer pattern methods
    virtual void attachObserver(ObserverPtr observer) = 0;
    virtual void detachObserver(ObserverPtr observer) = 0;
    virtual void notifyObservers(const std::string& eventType) = 0;
    
    // Strategy pattern methods
    virtual void applyFormatting(const std::string& strategyName) = 0;
    virtual void applyRendering(const std::string& strategyName) = 0;
    virtual void applyLayout(const std::string& strategyName) = 0;
};

// Concrete document types
class PdfDocument;
class WordDocument;
class TextDocument;

// Document processing tools
class DocumentViewer;
class DocumentEditor;
class DocumentConverter;

// Event types for observer pattern
namespace events {
    constexpr const char* DocumentChanged = "documentChanged";
    constexpr const char* DocumentSaved = "documentSaved";
    constexpr const char* DocumentLoaded = "documentLoaded";
}

} // namespace doc_system
