@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "Implement a flexible workflow engine using behavioral patterns"
@context "Modern C++20 implementation with event-driven architecture"

# Foundation layer pattern
@architecture foundation "event_driven"
"Components communicate through events and message handlers"

# Behavioral patterns - core workflow patterns
@architecture interaction "chain"
"handlers: [\"AuthenticationHandler\", \"ValidationHandler\", \"ProcessingHandler\", \"NotificationHandler\"]"

@architecture interaction "command"
"commands: [\"CreateWorkflow\", \"AssignTask\", \"CompleteTask\", \"CancelWorkflow\"]"

@architecture interaction "observer"
"events: [\"workflowCreated\", \"taskAssigned\", \"taskCompleted\", \"workflowCancelled\"]"

@architecture interaction "state"
"states: [\"Draft\", \"InProgress\", \"UnderReview\", \"Completed\", \"Cancelled\"]"

@architecture interaction "strategy"
"strategies: [\"DefaultPrioritization\", \"DeadlinePrioritization\", \"CostPrioritization\"]"

# Optional behavioral patterns - advanced workflow features
@architecture interaction "mediator"
"colleagues: [\"WorkflowManager\", \"UserManager\", \"NotificationService\"]"

@architecture interaction "memento"
"state_attributes: [\"taskAssignments\", \"currentState\", \"completedSteps\"]"

@architecture interaction "visitor"
"visitable_elements: [\"Task\", \"Workflow\", \"User\", \"Notification\"]"

# Component layer patterns for supporting implementation
@architecture component "factory_method"
"products: [\"Workflow\", \"Task\", \"User\", \"Notification\"]"

@architecture component "composite"
"component_type: \"WorkflowStep\""

@test "Chain of Responsibility routes requests through proper handler sequence"
@test "Command pattern correctly encapsulates operations on workflows"
@test "Observer pattern properly notifies relevant components of workflow changes"
@test "State pattern transitions workflow through its lifecycle states"
@test "Strategy pattern applies different prioritization algorithms"
@test "Mediator coordinates communication between workflow components"
@test "Memento captures and restores workflow state for undo/redo functionality"
@test "Visitor permits operations across different workflow element types"

@example "Workflow Execution Example" "
// Create a new workflow using factory
auto workflow = WorkflowFactory::createWorkflow(\"Approval\");

// Add tasks to the workflow using composite pattern
workflow->addStep(std::make_shared<ApprovalStep>(\"Initial Review\"));
workflow->addStep(std::make_shared<ApprovalStep>(\"Manager Approval\"));
workflow->addStep(std::make_shared<ApprovalStep>(\"Final Verification\"));

// Register observers for workflow events
workflow->attachObserver(std::make_shared<NotificationObserver>());
workflow->attachObserver(std::make_shared<AuditLogObserver>());

// Save the current state using memento pattern
auto checkpoint = workflow->createMemento();

// Execute a command on the workflow
CommandProcessor::execute(std::make_shared<AssignTaskCommand>(workflow, \"Initial Review\", user));

// Process the request through chain of responsibility
RequestHandler::process(workflowRequest);

// Switch prioritization strategy
workflow->setPrioritizationStrategy(std::make_shared<DeadlinePrioritization>());

// Apply a visitor to all elements in the workflow
WorkflowReportVisitor visitor;
workflow->accept(visitor);
"

@model "claude-3-sonnet"
@format "markdown"