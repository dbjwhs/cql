// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "document_types.h"
#include <string>
#include <memory>
#include <vector>
#include <algorithm>

namespace doc_system {

// Forward declaration of Document
class Document;

// Observer Pattern: DocumentObserver interface
class DocumentObserver {
public:
    virtual ~DocumentObserver() = default;
    
    // Called when a document event occurs
    virtual void onNotify(const Document& document, const std::string& eventType) = 0;
};

// Base document implementation with observer pattern support
class BaseDocument : public Document {
public:
    explicit BaseDocument(const std::string& filename) : filename_(filename) {}
    
    // Observer pattern implementation
    void attachObserver(ObserverPtr observer) override {
        if (observer) {
            auto it = std::find(observers_.begin(), observers_.end(), observer);
            if (it == observers_.end()) {
                observers_.push_back(observer);
            }
        }
    }
    
    void detachObserver(ObserverPtr observer) override {
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer),
            observers_.end()
        );
    }
    
    void notifyObservers(const std::string& eventType) override {
        for (const auto& observer : observers_) {
            observer->onNotify(*this, eventType);
        }
    }
    
    // Implement common methods from Document interface
    std::string getFilename() const override {
        return filename_;
    }

    void updateContent(const std::string& newContent) override {
        content_ = newContent;
        notifyObservers(events::DocumentChanged);
    }
    
    void setHeader(const std::string& header) override {
        header_ = header;
    }
    
    void setContent(const std::string& content) override {
        content_ = content;
    }
    
    void setFooter(const std::string& footer) override {
        footer_ = footer;
    }
    
    void setMetadata(const MetadataMap& metadata) override {
        metadata_ = metadata;
    }

protected:
    std::string filename_;
    std::string header_;
    std::string content_;
    std::string footer_;
    MetadataMap metadata_;
    std::vector<ObserverPtr> observers_;
};

// Concrete observer implementations

// Auto-save observer
class AutoSaveObserver : public DocumentObserver {
public:
    void onNotify(const Document& document, const std::string& eventType) override {
        if (eventType == events::DocumentChanged) {
            std::cout << "AutoSaveObserver: Auto-saving document " 
                      << document.getFilename() << std::endl;
            // In a real implementation, we would call document.save()
            // but we don't want to modify the const document in this method
        }
    }
};

// Validation observer
class ValidationObserver : public DocumentObserver {
public:
    void onNotify(const Document& document, const std::string& eventType) override {
        if (eventType == events::DocumentChanged) {
            std::cout << "ValidationObserver: Validating document " 
                      << document.getFilename() << std::endl;
            // Perform validation logic here
        }
    }
};

// Logging observer
class LoggingObserver : public DocumentObserver {
public:
    void onNotify(const Document& document, const std::string& eventType) override {
        std::cout << "LoggingObserver: Event '" << eventType 
                  << "' occurred on document " << document.getFilename() << std::endl;
    }
};

} // namespace doc_system
