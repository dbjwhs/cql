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

### Structural Patterns

| Pattern | Directive | Description | Parameters |
|---------|-----------|-------------|------------|
| Adapter | `@architecture component "adapter"` | Makes incompatible interfaces compatible | `adaptee: string, interface: string` |
| Bridge | `@architecture component "bridge"` | Separates abstraction from implementation | `implementors: string[]` |
| Composite | `@architecture component "composite"` | Composes objects into tree structures | `component_type: string` |
| Decorator | `@architecture component "decorator"` | Adds responsibilities to objects dynamically | `decorations: string[]` |
| Facade | `@architecture component "facade"` | Provides unified interface to a set of interfaces | `subsystems: string[]` |
| Flyweight | `@architecture component "flyweight"` | Shares common state between multiple objects | `shared_state: string[], unique_state: string[]` |
| Proxy | `@architecture component "proxy"` | Provides surrogate for another object | `proxy_type: string` |

### Behavioral Patterns

| Pattern | Directive | Description | Parameters |
|---------|-----------|-------------|------------|
| Chain of Responsibility | `@architecture interaction "chain"` | Passes requests along a chain | `handlers: string[]` |
| Command | `@architecture interaction "command"` | Encapsulates a request as an object | `commands: string[]` |
| Interpreter | `@architecture interaction "interpreter"` | Defines grammar for instructions | `grammar_rules: string[]` |
| Iterator | `@architecture interaction "iterator"` | Accesses elements sequentially | `collection_type: string` |
| Mediator | `@architecture interaction "mediator"` | Defines how objects interact | `colleagues: string[]` |
| Memento | `@architecture interaction "memento"` | Captures and restores object state | `state_attributes: string[]` |
| Observer | `@architecture interaction "observer"` | Notifies dependents of changes | `events: string[]` |
| State | `@architecture interaction "state"` | Alters object behavior when state changes | `states: string[]` |
| Strategy | `@architecture interaction "strategy"` | Encapsulates interchangeable algorithms | `strategies: string[]` |
| Template Method | `@architecture interaction "template_method"` | Defines skeleton of an algorithm | `steps: string[]` |
| Visitor | `@architecture interaction "visitor"` | Separates algorithm from object structure | `visitable_elements: string[]` |

### Compatibility Rules

The pattern compatibility system ensures that patterns work well together:

1. Only one foundation pattern allowed per query
2. Patterns in different layers are always compatible
3. Specific compatibility rules apply to patterns within the same layer:

   #### Creational Pattern Rules
   - Singleton and Prototype are incompatible (conflicting object creation models)
   - Factory Method and Abstract Factory work well together
   - Builder works well with Factory patterns
   
   #### Structural Pattern Rules
   - Bridge and Composite are incompatible in some implementations
   - Decorator works well with Composite (tree structure decoration)
   - Flyweight conflicts with Prototype (sharing vs. copying)
   - Adapter, Decorator, and Proxy are compatible with most patterns
   - Facade works well with complex subsystems using other patterns
   
   #### Behavioral Pattern Rules
   - Command works well with Memento for undo/redo operations
   - Observer and Mediator can overlap in communication responsibilities
   - State and Strategy should be carefully separated to avoid confusion
   - Visitor works well with Composite for complex tree traversals
   - Chain of Responsibility pairs well with Command and Composite
   - Template Method may overlap with Strategy's responsibilities

## Examples

### Creational Patterns Example

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

### Structural Patterns Example

```
@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "Implement a flexible UI component system"
@context "Modern C++20 implementation with component-based design"

# Foundation layer pattern
@architecture foundation "layered"
"Separate UI components from business logic and data sources"

# Component layer patterns - Structural patterns
@architecture component "composite"
"component_type: \"UIComponent\""

@architecture component "decorator"
"decorations: [\"Border\", \"Shadow\", \"ScrollBar\"]"

@architecture component "flyweight"
"shared_state: [\"Theme\", \"FontFamily\"], unique_state: [\"Text\", \"Position\"]"

# Use a factory to create components
@architecture component "factory_method"
"products: [\"Button\", \"TextField\", \"Panel\"]"

@test "Composite allows building complex UI hierarchies"
@test "Decorator dynamically adds visual effects to components"
@test "Flyweight reduces memory usage for repeated UI elements"
```

### Behavioral Patterns Example

```
@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "Implement a workflow management system"
@context "Modern C++20 implementation with event-driven design"

# Foundation layer pattern
@architecture foundation "event_driven"
"Components communicate via events and callbacks"

# Interaction layer patterns - Behavioral patterns
@architecture interaction "observer"
"events: [\"workflowCreated\", \"taskAssigned\", \"taskCompleted\"]"

@architecture interaction "command"
"commands: [\"CreateWorkflow\", \"AssignTask\", \"CompleteTask\"]"

@architecture interaction "state"
"states: [\"Draft\", \"InProgress\", \"UnderReview\", \"Completed\"]"

@architecture interaction "chain"
"handlers: [\"ValidationHandler\", \"SecurityHandler\", \"ProcessingHandler\"]"

# Component layer patterns for implementation
@architecture component "factory_method"
"products: [\"Workflow\", \"Task\", \"Notification\"]"

@test "Observer pattern notifies relevant components of workflow changes"
@test "Command pattern encapsulates workflow operations"
@test "State pattern manages workflow lifecycle properly"
@test "Chain of responsibility validates and processes workflow requests"
```

## Legacy Format Support

The traditional format is still supported for backward compatibility:

```
@architecture "Microservices with message queue"
@architecture "Each worker node is responsible for own state"
```

However, the new layered format is recommended for new queries as it provides better structure and validation.