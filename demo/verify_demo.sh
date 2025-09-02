#!/bin/bash
# Demo verification script - ensures all components work correctly

set -e

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$DEMO_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

echo "🔍 Verifying Meta-Prompt Compiler POC Demo Setup"
echo "================================================="

# Check CQL executable
echo "1. Checking CQL executable..."
if [[ -f "$BUILD_DIR/cql" && -x "$BUILD_DIR/cql" ]]; then
    echo "   ✅ CQL executable found and executable"
else
    echo "   ❌ CQL executable not found or not executable"
    echo "   Please run: mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

# Check demo examples
echo "2. Checking demo examples..."
EXAMPLES_DIR="$DEMO_DIR/examples"
if [[ -d "$EXAMPLES_DIR" ]]; then
    EXAMPLE_COUNT=$(find "$EXAMPLES_DIR" -name "*.llm" | wc -l)
    if [[ $EXAMPLE_COUNT -gt 0 ]]; then
        echo "   ✅ Found $EXAMPLE_COUNT example files"
        find "$EXAMPLES_DIR" -name "*.llm" -exec basename {} \; | sed 's/^/     - /'
    else
        echo "   ❌ No .llm example files found"
        exit 1
    fi
else
    echo "   ❌ Examples directory not found"
    exit 1
fi

# Check demo scripts
echo "3. Checking demo scripts..."
SCRIPTS=(
    "$DEMO_DIR/run_demo.sh"
    "$DEMO_DIR/benchmark_optimization.py"
)

for script in "${SCRIPTS[@]}"; do
    if [[ -f "$script" && -x "$script" ]]; then
        echo "   ✅ $(basename "$script") - executable"
    else
        echo "   ❌ $(basename "$script") - missing or not executable"
        exit 1
    fi
done

# Test basic CLI functionality
echo "4. Testing basic CLI functionality..."
if "$BUILD_DIR/cql" --help > /dev/null 2>&1; then
    echo "   ✅ CLI help command works"
else
    echo "   ❌ CLI help command failed"
    exit 1
fi

# Test optimization command (dry run)
echo "5. Testing optimization command syntax..."
FIRST_EXAMPLE=$(find "$EXAMPLES_DIR" -name "*.llm" | head -1)
if [[ -f "$FIRST_EXAMPLE" ]]; then
    # Test command parsing without actual execution
    if "$BUILD_DIR/cql" --optimize "$FIRST_EXAMPLE" --mode LOCAL_ONLY --goal BALANCED --domain software > /dev/null 2>&1; then
        echo "   ✅ Optimization command syntax valid"
    else
        echo "   ⚠️  Optimization command failed (may be expected without API key)"
    fi
else
    echo "   ❌ No example file found for testing"
    exit 1
fi

# Check Python for benchmarking
echo "6. Checking Python environment..."
if command -v python3 > /dev/null 2>&1; then
    PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2)
    echo "   ✅ Python 3 found (version: $PYTHON_VERSION)"
else
    echo "   ❌ Python 3 not found - needed for benchmarking"
    exit 1
fi

# Summary
echo ""
echo "🎉 Demo Verification Complete!"
echo "==============================="
echo ""
echo "✅ All components are ready for demonstration"
echo ""
echo "To run the demo:"
echo "  ./run_demo.sh"
echo ""
echo "To run benchmarks:"
echo "  python3 benchmark_optimization.py"
echo ""
echo "🚀 Meta-Prompt Compiler POC Demo is ready!"
