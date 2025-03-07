// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "document_types.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace doc_system {

// Strategy Pattern: Base strategy interfaces

// Text formatting strategy
class TextFormattingStrategy {
public:
    virtual ~TextFormattingStrategy() = default;
    virtual void format(Document& document) = 0;
    
    // Factory method to create formatting strategies
    static std::unique_ptr<TextFormattingStrategy> create(const std::string& strategyType);
};

// Image rendering strategy
class ImageRenderingStrategy {
public:
    virtual ~ImageRenderingStrategy() = default;
    virtual void render(Document& document) = 0;
    
    // Factory method to create rendering strategies
    static std::unique_ptr<ImageRenderingStrategy> create(const std::string& strategyType);
};

// Table layout strategy
class TableLayoutStrategy {
public:
    virtual ~TableLayoutStrategy() = default;
    virtual void layout(Document& document) = 0;
    
    // Factory method to create layout strategies
    static std::unique_ptr<TableLayoutStrategy> create(const std::string& strategyType);
};

// Concrete formatting strategies
class CorporateFormattingStrategy : public TextFormattingStrategy {
public:
    void format(Document& document) override {
        std::cout << "Applying corporate formatting to " << document.getFilename() << std::endl;
        // Apply corporate formatting rules
    }
};

class AcademicFormattingStrategy : public TextFormattingStrategy {
public:
    void format(Document& document) override {
        std::cout << "Applying academic formatting to " << document.getFilename() << std::endl;
        // Apply academic formatting rules
    }
};

// Concrete rendering strategies
class HighQualityRenderingStrategy : public ImageRenderingStrategy {
public:
    void render(Document& document) override {
        std::cout << "Applying high-quality rendering to " << document.getFilename() << std::endl;
        // Apply high-quality rendering
    }
};

class FastRenderingStrategy : public ImageRenderingStrategy {
public:
    void render(Document& document) override {
        std::cout << "Applying fast rendering to " << document.getFilename() << std::endl;
        // Apply fast rendering
    }
};

// Concrete layout strategies
class GridLayoutStrategy : public TableLayoutStrategy {
public:
    void layout(Document& document) override {
        std::cout << "Applying grid layout to " << document.getFilename() << std::endl;
        // Apply grid layout
    }
};

class CompactLayoutStrategy : public TableLayoutStrategy {
public:
    void layout(Document& document) override {
        std::cout << "Applying compact layout to " << document.getFilename() << std::endl;
        // Apply compact layout
    }
};

// Strategy Registry using type erasure pattern
class StrategyRegistry {
public:
    // Singleton access
    static StrategyRegistry& getInstance() {
        static StrategyRegistry instance;
        return instance;
    }
    
    // Register a formatting strategy factory
    template<typename T>
    void registerFormattingStrategy(const std::string& name) {
        static_assert(std::is_base_of<TextFormattingStrategy, T>::value, 
                     "Type must derive from TextFormattingStrategy");
        
        formattingStrategies_[name] = []() {
            return std::make_unique<T>();
        };
    }
    
    // Get a formatting strategy by name
    std::unique_ptr<TextFormattingStrategy> createFormattingStrategy(const std::string& name) {
        auto it = formattingStrategies_.find(name);
        if (it != formattingStrategies_.end()) {
            return it->second();
        }
        throw std::invalid_argument("Unknown formatting strategy: " + name);
    }
    
    // Similar methods for rendering and layout strategies
    template<typename T>
    void registerRenderingStrategy(const std::string& name) {
        static_assert(std::is_base_of<ImageRenderingStrategy, T>::value, 
                     "Type must derive from ImageRenderingStrategy");
        
        renderingStrategies_[name] = []() {
            return std::make_unique<T>();
        };
    }
    
    std::unique_ptr<ImageRenderingStrategy> createRenderingStrategy(const std::string& name) {
        auto it = renderingStrategies_.find(name);
        if (it != renderingStrategies_.end()) {
            return it->second();
        }
        throw std::invalid_argument("Unknown rendering strategy: " + name);
    }
    
    template<typename T>
    void registerLayoutStrategy(const std::string& name) {
        static_assert(std::is_base_of<TableLayoutStrategy, T>::value, 
                     "Type must derive from TableLayoutStrategy");
        
        layoutStrategies_[name] = []() {
            return std::make_unique<T>();
        };
    }
    
    std::unique_ptr<TableLayoutStrategy> createLayoutStrategy(const std::string& name) {
        auto it = layoutStrategies_.find(name);
        if (it != layoutStrategies_.end()) {
            return it->second();
        }
        throw std::invalid_argument("Unknown layout strategy: " + name);
    }
    
private:
    StrategyRegistry() {
        // Register default strategies
        registerFormattingStrategy<CorporateFormattingStrategy>("corporate");
        registerFormattingStrategy<AcademicFormattingStrategy>("academic");
        
        registerRenderingStrategy<HighQualityRenderingStrategy>("high-quality");
        registerRenderingStrategy<FastRenderingStrategy>("fast");
        
        registerLayoutStrategy<GridLayoutStrategy>("grid");
        registerLayoutStrategy<CompactLayoutStrategy>("compact");
    }
    
    std::unordered_map<std::string, std::function<std::unique_ptr<TextFormattingStrategy>()>> formattingStrategies_;
    std::unordered_map<std::string, std::function<std::unique_ptr<ImageRenderingStrategy>()>> renderingStrategies_;
    std::unordered_map<std::string, std::function<std::unique_ptr<TableLayoutStrategy>()>> layoutStrategies_;
};

// Implementation of factory methods
inline std::unique_ptr<TextFormattingStrategy> TextFormattingStrategy::create(const std::string& strategyType) {
    return StrategyRegistry::getInstance().createFormattingStrategy(strategyType);
}

inline std::unique_ptr<ImageRenderingStrategy> ImageRenderingStrategy::create(const std::string& strategyType) {
    return StrategyRegistry::getInstance().createRenderingStrategy(strategyType);
}

inline std::unique_ptr<TableLayoutStrategy> TableLayoutStrategy::create(const std::string& strategyType) {
    return StrategyRegistry::getInstance().createLayoutStrategy(strategyType);
}

} // namespace doc_system
