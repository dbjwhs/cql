// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "document_types.h"
#include <memory>
#include <stdexcept>

namespace doc_system {

// Abstract Product interfaces for document tools

// Document viewer interface
class DocumentViewer {
public:
    virtual ~DocumentViewer() = default;
    virtual void view(const DocumentPtr& document) = 0;
    virtual DocumentType getSupportedType() const = 0;
};

// Document editor interface
class DocumentEditor {
public:
    virtual ~DocumentEditor() = default;
    virtual void edit(DocumentPtr document) = 0;
    virtual DocumentType getSupportedType() const = 0;
};

// Document converter interface
class DocumentConverter {
public:
    virtual ~DocumentConverter() = default;
    virtual DocumentPtr convert(const DocumentPtr& document, DocumentType targetType) = 0;
    virtual DocumentType getSourceType() const = 0;
};

// Type aliases for tool pointers
using ViewerPtr = std::shared_ptr<DocumentViewer>;
using EditorPtr = std::shared_ptr<DocumentEditor>;
using ConverterPtr = std::shared_ptr<DocumentConverter>;

// Forward declarations of concrete factories
class PdfToolFactory;
class WordToolFactory;
class TextToolFactory;

// Abstract Factory: Tool factory interface
class DocumentToolFactory {
public:
    virtual ~DocumentToolFactory() = default;

    // Create viewer, editor, and converter for specific document types
    virtual ViewerPtr createViewer() const = 0;
    virtual EditorPtr createEditor() const = 0;
    virtual ConverterPtr createConverter() const = 0;

    // Factory method to create appropriate tool factory for document type
    static std::unique_ptr<DocumentToolFactory> createFactory(DocumentType type);
};

// Concrete factories for different document types

// PDF document tool factory
class PdfToolFactory : public DocumentToolFactory {
public:
    ViewerPtr createViewer() const override;
    EditorPtr createEditor() const override;
    ConverterPtr createConverter() const override;
};

// Word document tool factory
class WordToolFactory : public DocumentToolFactory {
public:
    ViewerPtr createViewer() const override;
    EditorPtr createEditor() const override;
    ConverterPtr createConverter() const override;
};

// Text document tool factory
class TextToolFactory : public DocumentToolFactory {
public:
    ViewerPtr createViewer() const override;
    EditorPtr createEditor() const override;
    ConverterPtr createConverter() const override;
};

// Client-side factory wrapper
class ToolFactory {
public:
    // Create a complete set of tools for a specific document type
    static std::tuple<ViewerPtr, EditorPtr, ConverterPtr> createToolsFor(DocumentType type) {
        auto factory = DocumentToolFactory::createFactory(type);
        return std::make_tuple(
            factory->createViewer(),
            factory->createEditor(),
            factory->createConverter()
        );
    }
};

} // namespace doc_system