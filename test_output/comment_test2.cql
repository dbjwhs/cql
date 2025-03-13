# This is a single line comment
@copyright "MIT" "dbjwhs"
@language "C++"
@description "This is a test file to check comment functionality" 

# This is a multi-line comment
# that spans several lines
# to demonstrate how the lexer
# handles consecutive comment lines
@context "Testing # character in context string"

@example "
This is an example code block
# This isn't a comment, it's part of the string
"

# Comments can appear between declarations
@variable "name" "value" 

# Another multi-line comment block
# with information about the following
# variable definition
@variable "debug" "true"
