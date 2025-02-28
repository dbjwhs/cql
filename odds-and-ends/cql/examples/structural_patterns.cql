@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "Implement a flexible UI component system with structural patterns"
@context "Modern C++20 implementation with component-based design"

# Foundation layer pattern
@architecture foundation "layered"
"Separate UI components from business logic and data sources"

# Component layer patterns - Structural patterns
@architecture component "adapter"
"interface: \"DatabaseInterface\", adaptee: \"LegacyDatabase\""

@architecture component "bridge"
"implementors: [\"WindowsRenderer\", \"MacOSRenderer\", \"LinuxRenderer\"]"

@architecture component "composite"
"component_type: \"UIComponent\""

@architecture component "decorator"
"decorations: [\"Border\", \"Shadow\", \"ScrollBar\"]"

@architecture component "facade"
"subsystems: [\"Networking\", \"DataStorage\", \"Authentication\"]"

@architecture component "flyweight"
"shared_state: [\"Theme\", \"FontFamily\"], unique_state: [\"Text\", \"Position\"]"

@architecture component "proxy"
"proxy_type: \"LazyLoadingProxy\""

# Creational pattern for component creation
@architecture component "factory_method"
"products: [\"Button\", \"TextField\", \"Panel\", \"Dialog\"]"

# Interaction pattern for component communications
@architecture interaction "observer"
"events: [\"click\", \"focus\", \"resize\", \"dataChange\"]"

@test "Adapter successfully connects legacy database to modern interface"
@test "Bridge separates component abstraction from platform-specific renderers"
@test "Composite allows building complex UI hierarchies"
@test "Decorator dynamically adds visual effects to components"
@test "Facade simplifies complex subsystem interactions"
@test "Flyweight reduces memory usage for repeated UI elements"
@test "Proxy provides lazy loading for expensive UI components"

@example "Composite + Decorator Example" "
// Create a composite UI panel
auto panel = ComponentFactory::createComponent(\"Panel\");

// Add child components to the panel
panel->addChild(ComponentFactory::createComponent(\"Button\"));
panel->addChild(ComponentFactory::createComponent(\"TextField\"));

// Use decorators to enhance components
auto decoratedPanel = std::make_shared<BorderDecorator>(panel);
decoratedPanel = std::make_shared<ShadowDecorator>(decoratedPanel);

// Use bridge for platform-specific rendering
decoratedPanel->setRenderer(RendererFactory::createRenderer());
decoratedPanel->render();

// Use flyweight for common styles
ThemeManager::getInstance().applyTheme(decoratedPanel, \"DarkTheme\");

// Add observer for event handling
decoratedPanel->addObserver(std::make_shared<ComponentResizeObserver>());
"

@model "claude-3-sonnet"
@format "markdown"