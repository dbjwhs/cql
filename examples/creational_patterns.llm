@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "Implement a document processing system with pluggable document formats"
@context "Modern C++20 implementation with type-erasure patterns"

@architecture "factory_method(products: [\"PdfDocument\", \"WordDocument\", \"TextDocument\"])"
@architecture "singleton(thread_safe: true, name: \"DocumentManager\")"
@architecture "builder(steps: [\"Header\", \"Content\", \"Footer\", \"Metadata\"])"
@architecture "abstract_factory(product_families: [\"Viewers\", \"Editors\", \"Converters\"])"
@architecture "prototype(deep_copy: true)"

@test "Factory method creates correct document type based on file extension"
@test "Builder constructs documents in the correct order"
@test "Singleton ensures only one DocumentManager exists"
@test "Abstract factory creates compatible document tools"
@test "Prototype cloning preserves all document properties"

@example "Factory Method Usage" "
class DocumentFactory {
public:
    static std::unique_ptr<Document> createDocument(const std::string& filename) {
        if (ends_with(filename, \".pdf\")) {
            return std::make_unique<PdfDocument>();
        } else if (ends_with(filename, \".docx\")) {
            return std::make_unique<WordDocument>();
        } else {
            return std::make_unique<TextDocument>();
        }
    }
};
"

@example "Singleton Pattern" "
class DocumentManager {
private:
    DocumentManager() = default;
    static std::mutex s_mutex;
    static std::unique_ptr<DocumentManager> s_instance;
    
public:
    static DocumentManager& getInstance() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_instance) {
            s_instance = std::unique_ptr<DocumentManager>(new DocumentManager());
        }
        return *s_instance;
    }
    
    // Prevent cloning
    DocumentManager(const DocumentManager&) = delete;
    DocumentManager& operator=(const DocumentManager&) = delete;
};

std::mutex DocumentManager::s_mutex;
std::unique_ptr<DocumentManager> DocumentManager::s_instance = nullptr;
"

@model "claude-3-sonnet"
@format "markdown"