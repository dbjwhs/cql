// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "document_types.h"
#include "document_observer.h"
#include "document_strategy.h"
#include <iostream>

namespace doc_system {

// Concrete document implementations

// PDF Document
class PdfDocument : public BaseDocument {
public:
    explicit PdfDocument(const std::string& filename) : BaseDocument(filename) {}
    
    DocumentType getType() const override {
        return DocumentType::PDF;
    }
    
    void save() override {
        std::cout << "Saving PDF document: " << filename_ << std::endl;
        notifyObservers(events::DocumentSaved);
    }
    
    void load() override {
        std::cout << "Loading PDF document: " << filename_ << std::endl;
        notifyObservers(events::DocumentLoaded);
    }
    
    void applyFormatting(const std::string& strategyName) override {
        auto strategy = TextFormattingStrategy::create(strategyName);
        strategy->format(*this);
    }
    
    void applyRendering(const std::string& strategyName) override {
        auto strategy = ImageRenderingStrategy::create(strategyName);
        strategy->render(*this);
    }
    
    void applyLayout(const std::string& strategyName) override {
        auto strategy = TableLayoutStrategy::create(strategyName);
        strategy->layout(*this);
    }
};

// Word Document
class WordDocument : public BaseDocument {
public:
    explicit WordDocument(const std::string& filename) : BaseDocument(filename) {}
    
    DocumentType getType() const override {
        return DocumentType::Word;
    }
    
    void save() override {
        std::cout << "Saving Word document: " << filename_ << std::endl;
        notifyObservers(events::DocumentSaved);
    }
    
    void load() override {
        std::cout << "Loading Word document: " << filename_ << std::endl;
        notifyObservers(events::DocumentLoaded);
    }
    
    void applyFormatting(const std::string& strategyName) override {
        auto strategy = TextFormattingStrategy::create(strategyName);
        strategy->format(*this);
    }
    
    void applyRendering(const std::string& strategyName) override {
        auto strategy = ImageRenderingStrategy::create(strategyName);
        strategy->render(*this);
    }
    
    void applyLayout(const std::string& strategyName) override {
        auto strategy = TableLayoutStrategy::create(strategyName);
        strategy->layout(*this);
    }
};

// Text Document
class TextDocument : public BaseDocument {
public:
    explicit TextDocument(const std::string& filename) : BaseDocument(filename) {}
    
    DocumentType getType() const override {
        return DocumentType::Text;
    }
    
    void save() override {
        std::cout << "Saving Text document: " << filename_ << std::endl;
        notifyObservers(events::DocumentSaved);
    }
    
    void load() override {
        std::cout << "Loading Text document: " << filename_ << std::endl;
        notifyObservers(events::DocumentLoaded);
    }
    
    void applyFormatting(const std::string& strategyName) override {
        auto strategy = TextFormattingStrategy::create(strategyName);
        strategy->format(*this);
    }
    
    void applyRendering(const std::string& strategyName) override {
        auto strategy = ImageRenderingStrategy::create(strategyName);
        strategy->render(*this);
    }
    
    void applyLayout(const std::string& strategyName) override {
        auto strategy = TableLayoutStrategy::create(strategyName);
        strategy->layout(*this);
    }
};

} // namespace doc_system
