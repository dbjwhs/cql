#!/bin/bash

# CQL compiler script
# Clean output is now the default behavior - this script is kept for compatibility

if [ $# -eq 0 ]; then
    echo "Usage: $0 <input.llm> [output.txt]"
    echo "Compiles a CQL file (clean output is now default)"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# CQL binary is in the parent directory's build folder
CQL_BINARY="$SCRIPT_DIR/../build/cql"

if [ ! -f "$CQL_BINARY" ]; then
    echo "Error: CQL compiler not found at $CQL_BINARY"
    echo "Please run 'make' from the project root to build it."
    exit 1
fi

if [ -z "$OUTPUT_FILE" ]; then
    # Output to stdout (clean is now default)
    "$CQL_BINARY" "$INPUT_FILE"
else
    # Output to file (clean is now default)
    "$CQL_BINARY" "$INPUT_FILE" "$OUTPUT_FILE"
fi
