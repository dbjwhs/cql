#include <iostream>
#include <string>
#include <map>
#include "include/cql/cql.hpp"

int main() {
    std::cout << "Testing variable replacement in templates\n\n";
    
    try {
        // Create a map of variables
        std::map<std::string, std::string> variables;
        variables["class_name"] = "CustomThreadSafeQueue";
        variables["element_type"] = "std::string";
        variables["max_size"] = "5000";
        
        // Get the template manager
        cql::TemplateManager template_mgr;
        
        // Load a template and replace variables
        std::string result = template_mgr.instantiate_template("test_template", variables);
        
        // Show the result
        std::cout << "Template with replaced variables:\n";
        std::cout << "---------------------------------\n";
        std::cout << result << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
