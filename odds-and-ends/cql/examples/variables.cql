@language "C++"
@description "Implement ${class_name} class with the specified interfaces"

@variable "class_name" "ThreadSafeQueue"
@variable "max_size" "1000"
@variable "element_type" "int"

@context "Modern C++20 implementation"

@example "Usage" "
// Create a queue with limited capacity
${class_name}<${element_type}> queue(${max_size});

// Basic operations
queue.push(42);
auto value = queue.try_pop();
"

@test "Test with ${max_size} elements"
@test "Empty queue behavior"