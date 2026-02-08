#!/bin/bash
# Run unit tests for Meadows Compiler

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_EXECUTABLE="$BUILD_DIR/tests/meadows_tests"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows Compiler Unit Tests${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Detect LLVM
source "$SCRIPT_DIR/../dev/detect_llvm.sh"
LLVM_DIR="${LLVM_DIR:-$(get_llvm_dir)}"

# Build if needed
if [ ! -d "$BUILD_DIR" ] || [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${YELLOW}Building tests...${NC}"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. -DBUILD_TESTS=ON ${LLVM_DIR:+-DLLVM_DIR="${LLVM_DIR}"}
    NJOBS=$(get_cmake_jobs)
    cmake --build . --parallel "$NJOBS" --target meadows_tests
fi

# Check test executable
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${RED}Error: Test executable not found at $TEST_EXECUTABLE${NC}"
    exit 1
fi

# Run tests
echo
echo -e "${YELLOW}Running tests...${NC}"
echo

if "$TEST_EXECUTABLE" --reporter console; then
    echo
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}   All unit tests passed!${NC}"
    echo -e "${GREEN}========================================${NC}"
    exit 0
else
    echo
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}   Some tests failed!${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
