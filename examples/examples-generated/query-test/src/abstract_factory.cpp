#include "../headers/abstract_factory.h"
#include "../headers/concrete_tools.h"

namespace doc_system {

// Factory method implementation
std::unique_ptr<DocumentToolFactory> DocumentToolFactory::createFactory(DocumentType type) {
    switch (type) {
        case DocumentType::PDF:
            return std::make_unique<PdfToolFactory>();
        case DocumentType::Word:
            return std::make_unique<WordToolFactory>();
        case DocumentType::Text:
            return std::make_unique<TextToolFactory>();
        default:
            throw std::invalid_argument("Unsupported document type");
    }
}

// PDF tool factory implementation
ViewerPtr PdfToolFactory::createViewer() const {
    return std::make_shared<PdfViewer>();
}

EditorPtr PdfToolFactory::createEditor() const {
    return std::make_shared<PdfEditor>();
}

ConverterPtr PdfToolFactory::createConverter() const {
    return std::make_shared<PdfConverter>();
}

// Word tool factory implementation
ViewerPtr WordToolFactory::createViewer() const {
    return std::make_shared<WordViewer>();
}

EditorPtr WordToolFactory::createEditor() const {
    return std::make_shared<WordEditor>();
}

ConverterPtr WordToolFactory::createConverter() const {
    return std::make_shared<WordConverter>();
}

// Text tool factory implementation
ViewerPtr TextToolFactory::createViewer() const {
    return std::make_shared<TextViewer>();
}

EditorPtr TextToolFactory::createEditor() const {
    return std::make_shared<TextEditor>();
}

ConverterPtr TextToolFactory::createConverter() const {
    return std::make_shared<TextConverter>();
}

} // namespace doc_system