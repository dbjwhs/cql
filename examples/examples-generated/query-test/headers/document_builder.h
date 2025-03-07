// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "document_types.h"

namespace doc_system {

// Builder Pattern for constructing documents
class DocumentBuilder {
public:
    DocumentBuilder() = default;
    
    // Reset the builder to create a new document
    void reset(DocumentPtr document) {
        document_ = document;
        buildSteps_.clear();
    }
    
    // Builder steps - returns the builder for method chaining
    DocumentBuilder& buildHeader(const std::string& header) {
        if (document_) {
            document_->setHeader(header);
            buildSteps_.push_back("Header");
        }
        return *this;
    }
    
    DocumentBuilder& buildContent(const std::string& content) {
        if (document_) {
            document_->setContent(content);
            buildSteps_.push_back("Content");
        }
        return *this;
    }
    
    DocumentBuilder& buildFooter(const std::string& footer) {
        if (document_) {
            document_->setFooter(footer);
            buildSteps_.push_back("Footer");
        }
        return *this;
    }
    
    DocumentBuilder& buildMetadata(const MetadataMap& metadata) {
        if (document_) {
            document_->setMetadata(metadata);
            buildSteps_.push_back("Metadata");
        }
        return *this;
    }
    
    // Get the constructed document
    DocumentPtr getDocument() const {
        return document_;
    }
    
    // Get the order of build steps for testing
    const std::vector<std::string>& getBuildSteps() const {
        return buildSteps_;
    }

private:
    DocumentPtr document_;
    std::vector<std::string> buildSteps_;
};

// Director class to enforce specific document building workflows
class DocumentDirector {
public:
    // Builds a standard document with all components
    static void buildStandardDocument(DocumentBuilder& builder, 
                                     const std::string& header,
                                     const std::string& content,
                                     const std::string& footer,
                                     const MetadataMap& metadata) {
        builder.buildHeader(header)
               .buildContent(content)
               .buildFooter(footer)
               .buildMetadata(metadata);
    }
    
    // Builds a minimal document with just content
    static void buildMinimalDocument(DocumentBuilder& builder,
                                    const std::string& content) {
        builder.buildContent(content);
    }
    
    // Builds a business document with metadata first
    static void buildBusinessDocument(DocumentBuilder& builder,
                                     const std::string& header,
                                     const std::string& content,
                                     const MetadataMap& metadata) {
        builder.buildMetadata(metadata)
               .buildHeader(header)
               .buildContent(content);
    }
};

} // namespace doc_system
