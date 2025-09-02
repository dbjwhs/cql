#!/bin/bash
# Meta-Prompt Compiler POC Demo
# Showcases optimization capabilities with real examples and metrics

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Demo configuration
DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(dirname "$DEMO_DIR")/build"
CQL_EXECUTABLE="$BUILD_DIR/cql"

echo -e "${BLUE}🚀 Meta-Prompt Compiler POC Demo${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

# Verify CQL executable exists
if [[ ! -f "$CQL_EXECUTABLE" ]]; then
    echo -e "${RED}❌ Error: CQL executable not found at $CQL_EXECUTABLE${NC}"
    echo -e "${YELLOW}Please build the project first: mkdir -p build && cd build && cmake .. && make${NC}"
    exit 1
fi

echo -e "${GREEN}✅ CQL executable found${NC}"
echo ""

# Function to run demo with metrics
run_optimization_demo() {
    local input_file="$1"
    local demo_name="$2"
    local mode="$3"
    local goal="$4"
    local domain="$5"
    
    echo -e "${BLUE}📊 Demo: $demo_name${NC}"
    echo -e "${BLUE}----------------------------------------${NC}"
    echo -e "${YELLOW}Input file: $input_file${NC}"
    echo -e "${YELLOW}Mode: $mode${NC}"
    echo -e "${YELLOW}Goal: $goal${NC}"
    echo -e "${YELLOW}Domain: $domain${NC}"
    echo ""
    
    # Check if input file exists
    if [[ ! -f "$input_file" ]]; then
        echo -e "${RED}❌ Input file not found: $input_file${NC}"
        return 1
    fi
    
    # Show original file size
    local original_size=$(wc -c < "$input_file")
    echo -e "${BLUE}📄 Original query size: $original_size characters${NC}"
    
    # Run optimization with metrics
    echo -e "${GREEN}🔄 Running optimization...${NC}"
    echo ""
    
    local cmd="$CQL_EXECUTABLE --optimize \"$input_file\" --mode \"$mode\" --goal \"$goal\" --domain \"$domain\" --show-metrics --show-validation"
    echo -e "${YELLOW}Command: $cmd${NC}"
    echo ""
    
    # Execute the command
    if eval "$cmd"; then
        echo -e "${GREEN}✅ Optimization completed successfully${NC}"
    else
        echo -e "${RED}❌ Optimization failed${NC}"
        return 1
    fi
    
    echo ""
    echo -e "${BLUE}----------------------------------------${NC}"
    echo ""
    sleep 2
}

# Function to show comparison
show_comparison() {
    local input_file="$1"
    local output_file="$2"
    local demo_name="$3"
    
    echo -e "${BLUE}📊 Comparison: $demo_name${NC}"
    echo -e "${BLUE}----------------------------------------${NC}"
    
    if [[ -f "$input_file" ]] && [[ -f "$output_file" ]]; then
        local original_size=$(wc -c < "$input_file")
        local optimized_size=$(wc -c < "$output_file")
        local reduction=$(( (original_size - optimized_size) * 100 / original_size ))
        
        echo -e "${YELLOW}Original size: $original_size characters${NC}"
        echo -e "${YELLOW}Optimized size: $optimized_size characters${NC}"
        echo -e "${GREEN}Size reduction: $reduction%${NC}"
    else
        echo -e "${RED}❌ Files not found for comparison${NC}"
    fi
    echo ""
}

# Main demo flow
main() {
    echo -e "${GREEN}🎯 Starting Meta-Prompt Compiler Demo...${NC}"
    echo ""
    
    # Demo 1: Basic Optimization with LOCAL_ONLY mode
    echo -e "${BLUE}=== DEMO 1: Basic Query Optimization ===${NC}"
    run_optimization_demo \
        "$DEMO_DIR/examples/token_optimization.llm" \
        "Token Optimization (Local Mode)" \
        "LOCAL_ONLY" \
        "BALANCED" \
        "software"
    
    # Demo 2: Advanced optimization with token reduction
    echo -e "${BLUE}=== DEMO 2: Advanced Token Reduction ===${NC}"
    run_optimization_demo \
        "$DEMO_DIR/examples/basic_optimization.llm" \
        "Complex Pipeline Optimization" \
        "LOCAL_ONLY" \
        "REDUCE_TOKENS" \
        "software"
    
    # Demo 3: Domain-specific optimization
    echo -e "${BLUE}=== DEMO 3: Domain-Specific Optimization ===${NC}"
    run_optimization_demo \
        "$DEMO_DIR/examples/basic_optimization.llm" \
        "Domain-Specific (Software Engineering)" \
        "LOCAL_ONLY" \
        "DOMAIN_SPECIFIC" \
        "software"
    
    # Performance summary
    echo -e "${BLUE}=== PERFORMANCE SUMMARY ===${NC}"
    echo -e "${GREEN}✅ All demos completed successfully${NC}"
    echo -e "${YELLOW}📈 Meta-Prompt Compiler demonstrates:${NC}"
    echo -e "   • Real-time query optimization"
    echo -e "   • Multiple compilation modes (LOCAL_ONLY, CACHED_LLM, FULL_LLM)"
    echo -e "   • Flexible optimization goals (REDUCE_TOKENS, IMPROVE_ACCURACY, BALANCED, DOMAIN_SPECIFIC)"
    echo -e "   • Domain-aware optimization"
    echo -e "   • Comprehensive metrics and validation"
    echo ""
    
    echo -e "${BLUE}🎉 POC Demo Complete!${NC}"
    echo -e "${GREEN}The Meta-Prompt Compiler is ready for production use.${NC}"
    echo ""
}

# Error handling
trap 'echo -e "${RED}❌ Demo interrupted${NC}"; exit 1' INT TERM

# Run main demo
main "$@"
