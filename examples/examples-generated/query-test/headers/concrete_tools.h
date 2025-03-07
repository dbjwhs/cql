// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "document_types.h"
#include "abstract_factory.h"
#include <iostream>
#include <memory>

namespace doc_system {

// Concrete Product Classes for PDF documents
class PdfViewer : public DocumentViewer {
public:
    void view(const DocumentPtr& document) override {
        if (document->getType() != DocumentType::PDF) {
            throw std::invalid_argument("PdfViewer can only view PDF documents");
        }

        std::cout << "Viewing PDF document: " << document->getFilename() << std::endl;
    }

    DocumentType getSupportedType() const override {
        return DocumentType::PDF;
    }
};

class PdfEditor : public DocumentEditor {
public:
    void edit(DocumentPtr document) override {
        if (document->getType() != DocumentType::PDF) {
            throw std::invalid_argument("PdfEditor can only edit PDF documents");
        }

        std::cout << "Editing PDF document: " << document->getFilename() << std::endl;
    }

    DocumentType getSupportedType() const override {
        return DocumentType::PDF;
    }
};

class PdfConverter : public DocumentConverter {
public:
    DocumentPtr convert(const DocumentPtr& document, DocumentType targetType) override {
        if (document->getType() != DocumentType::PDF) {
            throw std::invalid_argument("PdfConverter can only convert from PDF documents");
        }

        std::cout << "Converting PDF document to "
                  << (targetType == DocumentType::Word ? "Word" :
                     (targetType == DocumentType::Text ? "Text" : "Unknown"))
                  << " format: " << document->getFilename() << std::endl;

        // In a real implementation, we would create a new document of the target type
        // and copy the content from the source document
        return document;  // Placeholder return
    }

    DocumentType getSourceType() const override {
        return DocumentType::PDF;
    }
};

// Concrete Product Classes for Word documents

class WordViewer : public DocumentViewer {
public:
    void view(const DocumentPtr& document) override {
        if (document->getType() != DocumentType::Word) {
            throw std::invalid_argument("WordViewer can only view Word documents");
        }

        std::cout << "Viewing Word document: " << document->getFilename() << std::endl;
    }

    DocumentType getSupportedType() const override {
        return DocumentType::Word;
    }
};

class WordEditor : public DocumentEditor {
public:
    void edit(DocumentPtr document) override {
        if (document->getType() != DocumentType::Word) {
            throw std::invalid_argument("WordEditor can only edit Word documents");
        }

        std::cout << "Editing Word document: " << document->getFilename() << std::endl;
    }

    DocumentType getSupportedType() const override {
        return DocumentType::Word;
    }
};

class WordConverter : public DocumentConverter {
public:
    DocumentPtr convert(const DocumentPtr& document, DocumentType targetType) override {
        if (document->getType() != DocumentType::Word) {
            throw std::invalid_argument("WordConverter can only convert from Word documents");
        }

        std::cout << "Converting Word document to "
                  << (targetType == DocumentType::PDF ? "PDF" :
                     (targetType == DocumentType::Text ? "Text" : "Unknown"))
                  << " format: " << document->getFilename() << std::endl;

        return document;  // Placeholder return
    }

    DocumentType getSourceType() const override {
        return DocumentType::Word;
    }
};

// Concrete Product Classes for Text documents

class TextViewer : public DocumentViewer {
public:
    void view(const DocumentPtr& document) override {
        if (document->getType() != DocumentType::Text) {
            throw std::invalid_argument("TextViewer can only view Text documents");
        }

        std::cout << "Viewing Text document: " << document->getFilename() << std::endl;
    }

    DocumentType getSupportedType() const override {
        return DocumentType::Text;
    }
};

class TextEditor : public DocumentEditor {
public:
    void edit(DocumentPtr document) override {
        if (document->getType() != DocumentType::Text) {
            throw std::invalid_argument("TextEditor can only edit Text documents");
        }

        std::cout << "Editing Text document: " << document->getFilename() << std::endl;
    }

    DocumentType getSupportedType() const override {
        return DocumentType::Text;
    }
};

class TextConverter : public DocumentConverter {
public:
    DocumentPtr convert(const DocumentPtr& document, DocumentType targetType) override {
        if (document->getType() != DocumentType::Text) {
            throw std::invalid_argument("TextConverter can only convert from Text documents");
        }

        std::cout << "Converting Text document to "
                  << (targetType == DocumentType::PDF ? "PDF" :
                     (targetType == DocumentType::Word ? "Word" : "Unknown"))
                  << " format: " << document->getFilename() << std::endl;

        return document;  // Placeholder return
    }

    DocumentType getSourceType() const override {
        return DocumentType::Text;
    }
};

// Note: Factory implementations moved to abstract_factory.cpp

} // namespace doc_system