# CQL Architecture Patterns

CQL now supports structured architecture pattern definitions with compatibility validation. This feature allows you to express architectural decisions explicitly in your queries.

## Layered Architecture Pattern System

The architecture pattern system is organized into three layers:

1. **Foundation Layer**: Core architectural patterns that define the overall system structure
   - Only one foundation pattern allowed per query
   - Examples: microservices, layered, event-driven, serverless

2. **Component Layer**: Patterns for individual components within the architecture
   - Multiple component patterns allowed with compatibility checking
   - Examples: factory_method, singleton, builder, adapter, facade

3. **Interaction Layer**: Patterns that govern how components interact with each other
   - Multiple interaction patterns allowed with compatibility checking
   - Examples: observer, mediator, command, strategy

## Syntax

The new layered syntax provides better structure and validation:

```
@architecture [layer] "[pattern_name]" "[optional_parameters]"
```

Examples:
```
@architecture foundation "microservices" "Components communicate via message queues"
@architecture component "factory_method" "products: [\"Document\", \"Image\"]"
@architecture component "singleton" "name: \"DocumentManager\", thread_safe: true"
@architecture interaction "observer" "events: [\"documentChanged\", \"documentSaved\"]"
```

## Supported Design Patterns

### Creational Patterns

| Pattern | Directive | Description | Parameters |
|---------|-----------|-------------|------------|
| Abstract Factory | `@architecture component "abstract_factory"` | Creates families of related objects | `product_families: string[]` |
| Builder | `@architecture component "builder"` | Separates object construction from representation | `steps: string[]` |
| Factory Method | `@architecture component "factory_method"` | Creates objects without specifying the exact class | `products: string[]` |
| Prototype | `@architecture component "prototype"` | Creates objects by cloning existing ones | `deep_copy: boolean` |
| Singleton | `@architecture component "singleton"` | Ensures a class has only one instance | `thread_safe: boolean, name: string` |

### Compatibility Rules

The pattern compatibility system ensures that patterns work well together:

1. Only one foundation pattern allowed per query
2. Patterns in different layers are always compatible
3. Specific compatibility rules apply to patterns within the same layer:
   - Singleton and Prototype are incompatible (conflicting object creation models)
   - Factory Method and Abstract Factory work well together
   - Builder works well with Factory patterns

## Example

Here's a complete example showing layered pattern usage:

```
@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "Implement a document processing system"
@context "Modern C++20 implementation"

# Foundation layer pattern
@architecture foundation "microservices"
"Components communicate via message queues"

# Component layer patterns
@architecture component "factory_method"
"products: [\"PdfDocument\", \"WordDocument\", \"TextDocument\"]"

@architecture component "singleton"
"name: \"DocumentManager\", thread_safe: true"

# Interaction layer patterns
@architecture interaction "observer"
"events: [\"documentChanged\", \"documentSaved\"]"

@test "Factory method creates correct document types"
@test "Singleton ensures only one DocumentManager exists"
@test "Observer pattern properly notifies subscribers"
```

## Legacy Format Support

The traditional format is still supported for backward compatibility:

```
@architecture "Microservices with message queue"
@architecture "Each worker node is responsible for own state"
```

However, the new layered format is recommended for new queries as it provides better structure and validation.