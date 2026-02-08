#!/bin/bash

# Static Analysis Script for Meadows Compiler
# Runs clang-tidy and cppcheck on source code

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LLVM_DIR="${LLVM_DIR:-$(llvm-config --cmakedir 2>/dev/null || echo '/usr/lib/llvm-17/lib/cmake/llvm')}"

echo "=== Meadows Compiler Static Analysis ==="
echo ""

# Detect available tools
CLANG_TIDY=$(which clang-tidy 2>/dev/null || echo "")
CPPCHECK=$(which cppcheck 2>/dev/null || echo "")
CLANG_EXPORT=$(which clang-export 2>/dev/null || echo "")

if [ -z "$CLANG_TIDY" ] && [ -z "$CPPCHECK" ]; then
    echo "Warning: No static analysis tools found."
    echo "Install with: brew install clang-tidy cppcheck"
    echo ""
    echo "Optional: sudo apt-get install clang-tidy cppcheck (Linux)"
    exit 0
fi

# Generate compile_commands.json if not exists
if [ ! -f "$PROJECT_ROOT/build/compile_commands.json" ]; then
    echo "Generating compile_commands.json..."
    cmake -B "$PROJECT_ROOT/build-analyze" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DLLVM_DIR="$LLVM_DIR" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        "$PROJECT_ROOT" 2>/dev/null || true
fi

ERRORS=0
WARNINGS=0

# Run clang-tidy if available
if [ -n "$CLANG_TIDY" ]; then
    echo "Running clang-tidy..."
    echo "------------------------"
    
    if [ -f "$PROJECT_ROOT/build/compile_commands.json" ]; then
        cd "$PROJECT_ROOT"
        
        # Run clang-tidy on source files
        clang-tidy \
            -p="$PROJECT_ROOT/build" \
            -config="$PROJECT_ROOT/.clang-tidy" \
            src/lexer/Lexer.cpp \
            src/parser/Parser.cpp \
            src/ast/AST.cpp \
            src/codegen/CodeGen.cpp \
            src/main/meadows.cpp \
            2>&1 | tee "$PROJECT_ROOT/build/clang_tidy_output.txt" || true
        
        # Count issues
        ERRORS=$(grep -c "error:" "$PROJECT_ROOT/build/clang_tidy_output.txt" 2>/dev/null || echo "0")
        WARNINGS=$(grep -c "warning:" "$PROJECT_ROOT/build/clang_tidy_output.txt" 2>/dev/null || echo "0")
        
        echo ""
        echo "clang-tidy: $ERRORS errors, $WARNINGS warnings"
    else
        echo "Warning: compile_commands.json not found. Skipping clang-tidy."
    fi
fi

# Run cppcheck if available
if [ -n "$CPPCHECK" ]; then
    echo ""
    echo "Running cppcheck..."
    echo "--------------------"
    
    cd "$PROJECT_ROOT"
    
    cppcheck \
        --enable=all \
        --force \
        --std=c++17 \
        --include="$LLVM_DIR" \
        src/ 2>&1 | tee "$PROJECT_ROOT/build/cppcheck_output.txt" || true
    
    # Count issues
    CPP_ERRORS=$(grep -c "error" "$PROJECT_ROOT/build/cppcheck_output.txt" 2>/dev/null || echo "0")
    CPP_WARNINGS=$(grep -c "warning" "$PROJECT_ROOT/build/cppcheck_output.txt" 2>/dev/null || echo "0")
    
    echo ""
    echo "cppcheck: $CPP_ERRORS errors, $CPP_WARNINGS warnings"
fi

# Summary
echo ""
echo "=== Analysis Summary ==="
echo ""

if [ -n "$CLANG_TIDY" ]; then
    echo "clang-tidy results saved to: build/clang_tidy_output.txt"
fi
if [ -n "$CPPCHECK" ]; then
    echo "cppcheck results saved to: build/cppcheck_output.txt"
fi

echo ""
echo "Note: Some warnings are expected due to LLVM header interactions."
echo "Focus on warnings in src/ directory."

# Return non-zero if there are errors
if [ "$ERRORS" -gt 0 ]; then
    exit 1
fi

exit 0
