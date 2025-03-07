// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "document_types.h"
#include "concrete_documents.h"
#include <filesystem>

namespace doc_system {

    // Factory Method Pattern: DocumentFactory creates different document types
    class DocumentFactory {
    public:
        // Creates a document based on file extension
        static DocumentPtr createDocument(const std::string& filename) {
            auto extension = std::filesystem::path(filename).extension().string();

            // Convert to lowercase for case-insensitive comparison
            std::transform(extension.begin(), extension.end(), extension.begin(),
                          [](unsigned char c) { return std::tolower(c); });

            if (extension == ".pdf") {
                return createPdfDocument(filename);
            } else if (extension == ".docx" || extension == ".doc") {
                return createWordDocument(filename);
            } else if (extension == ".txt") {
                return createTextDocument(filename);
            } else {
                throw std::invalid_argument(
                    std::format("Unsupported file extension: {}", extension));
            }
        }

        // Made these public and non-inline to ensure they are accessible
        static DocumentPtr createPdfDocument(const std::string& filename) {
            return std::make_shared<PdfDocument>(filename);
        }

        static DocumentPtr createWordDocument(const std::string& filename) {
            return std::make_shared<WordDocument>(filename);
        }

        static DocumentPtr createTextDocument(const std::string& filename) {
            return std::make_shared<TextDocument>(filename);
        }
    };

} // namespace doc_system