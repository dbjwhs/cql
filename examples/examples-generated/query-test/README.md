# Document Processing System

The code in this project was 95% developed using Claude with CQL (Claude Query Language), the file use was *architecture_patterns.cql* in the example directory of CQL.

```bash
cql ../examples/architecture_patterns.cql
```

This single query generate a modern C++20 document processing system demonstrating proper architecture patterns and design principles.

## Architecture Overview

This system implements several architectural patterns:

### Design Patterns

- **Factory Method**: Creates appropriate document types based on file extensions
- **Singleton**: Thread-safe DocumentManager for managing document instances
- **Builder**: Step-by-step document construction with chaining support
- **Abstract Factory**: Creates families of compatible document tools
- **Observer**: Notification system for document events
- **Strategy**: Interchangeable algorithms for document processing

### Communication

- **Message Queue**: Microservices communication infrastructure
- **Event-Driven Architecture**: Document processing components communicate via events

## Components

- `document_types.h`: Core interfaces and type definitions
- `document_factory.h`: Factory method implementation for document creation
- `document_manager.h`: Singleton document manager
- `document_builder.h`: Builder pattern for document construction
- `document_observer.h`: Observer pattern for document events
- `document_strategy.h`: Strategy pattern for formatting, rendering, and layout
- `abstract_factory.h`: Abstract factory for document tools
- `concrete_documents.h`: Concrete document implementations
- `concrete_tools.h`: Concrete tool implementations
- `message_queue.h`: Message queue infrastructure

## Usage Example

```cpp
// Create a document using factory method
auto document = DocumentFactory::createDocument("report.pdf");

// Use builder pattern to construct the document
DocumentBuilder builder;
builder.reset(document);
builder.buildHeader("Quarterly Report")
       .buildContent("This is the content of the report.")
       .buildFooter("Confidential")
       .buildMetadata({
           {"author", "Jane Doe"},
           {"created", "2025-02-28"}
       });

auto doc = builder.getDocument();

// Register with singleton document manager
DocumentManager::getInstance().registerDocument(doc);

// Create compatible tools using abstract factory
auto [viewer, editor, converter] = ToolFactory::createToolsFor(doc->getType());

// Apply strategy pattern
doc->applyFormatting("corporate");

// Observer pattern
doc->attachObserver(std::make_shared<AutoSaveObserver>());
doc->attachObserver(std::make_shared<ValidationObserver>());

// Update content to trigger observers
doc->updateContent("Updated content for the quarterly report.");
```

## Building and Testing

### Requirements

- C++20 compatible compiler (GCC 10+, Clang 10+, or MSVC 2019+)
- CMake 3.15 or higher
- GoogleTest (automatically downloaded by CMake)

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run all tests
ctest --output-on-failure
# or
cmake --build . --target run_all_tests

# Install
cmake --install .
```

### Build Options

- **CMAKE_BUILD_TYPE**: Set to Release, Debug, etc.
- **CMAKE_INSTALL_PREFIX**: Set installation directory

```bash
# Example with build options
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

## Testing

The system includes comprehensive tests for all architectural patterns:

- **Factory Method**: Tests correct document creation based on file extensions
- **Builder**: Tests document construction in correct order
- **Singleton**: Tests that only one DocumentManager exists and is thread-safe
- **Abstract Factory**: Tests creation of compatible document tools
- **Observer**: Tests proper notification of all registered components
- **Strategy**: Tests interchangeable formatting, rendering, and layout algorithms

## Documentation

Doxygen can generate API documentation:


```bash
# mac osx
brew install doxygen

# Generate documentation
cmake --build . --target docs
```

## License

MIT License
Copyright (c) 2025 dbjwhs
