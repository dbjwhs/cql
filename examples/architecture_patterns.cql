@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "Implement a document processing system with proper architecture patterns"
@context "Modern C++20 implementation with type-erasure patterns"

# Foundation layer pattern (sets the overall architecture)
@architecture foundation "microservices"
"Components communicate via message queues with well-defined interfaces"

# Component layer patterns (define individual components)
@architecture component "factory_method"
"products: [\"PdfDocument\", \"WordDocument\", \"TextDocument\"]"

@architecture component "singleton"
"name: \"DocumentManager\", thread_safe: true"

@architecture component "builder"
"steps: [\"Header\", \"Content\", \"Footer\", \"Metadata\"]"

@architecture component "abstract_factory"
"product_families: [\"Viewers\", \"Editors\", \"Converters\"]"

# Interaction layer patterns (define how components interact)
@architecture interaction "observer"
"events: [\"documentChanged\", \"documentSaved\", \"documentLoaded\"]"

@architecture interaction "strategy"
"strategies: [\"TextFormatting\", \"ImageRendering\", \"TableLayout\"]"

@test "Factory method creates correct document type based on file extension"
@test "Builder constructs documents in the correct order"
@test "Singleton ensures only one DocumentManager exists"
@test "Abstract factory creates compatible document tools"
@test "Observer pattern properly notifies all registered components on document changes"

@example "Integration Example" "
// Create a document using the factory
auto document = DocumentFactory::createDocument(\"report.pdf\");

// Build document content with the builder
DocumentBuilder builder;
builder.buildHeader(\"Quarterly Report\")
       .buildContent(content_data)
       .buildFooter(\"Confidential\")
       .buildMetadata({\"author\": \"Jane Doe\", \"created\": \"2025-02-28\"});
       
auto doc = builder.getDocument();

// Document Manager singleton tracks all documents
DocumentManager::getInstance().registerDocument(doc);

// Document tools created via abstract factory
auto documentTools = ToolFactory::createToolsFor(doc->getType());

// Use strategy pattern to apply formatting
doc->applyFormatting(TextFormattingStrategy::create(\"corporate\"));

// Attach observers
doc->attachObserver(std::make_shared<AutoSaveObserver>());
doc->attachObserver(std::make_shared<ValidationObserver>());

// Later, when document changes
doc->updateContent(new_content);  // Observers are automatically notified
"

@model "claude-3-opus"
@format "markdown"