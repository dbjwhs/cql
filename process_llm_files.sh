#!/bin/bash

# Directory containing .llm files
INPUT_DIR="examples"
# Base directory for output
OUTPUT_BASE_DIR="examples/examples-generated"

# Make sure the output base directory exists
mkdir -p "$OUTPUT_BASE_DIR"

# Find all .llm files in the input directory
find "$INPUT_DIR" -type f -name "*.llm" | while read -r llm_file; do
    # Extract just the filename without path
    filename=$(basename "$llm_file")

    # Remove .llm extension to get the base name
    base_name="${filename%.llm}"

    # Create the output directory
    output_dir="$OUTPUT_BASE_DIR/$base_name"
    mkdir -p "$output_dir"

    # Define the output file path
    output_file="$output_dir/$base_name.txt"

    # Run the cql command
    echo "Processing $llm_file -> $output_file"
    build/cql "$llm_file" > "$output_file"
done

echo "Processing complete"

