#!/bin/bash

# Unit Test Runner for Meadows Compiler
# Runs all unit tests and reports results

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

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Build directory not found. Building...${NC}"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. -DENABLE_TESTING=ON -DLLVM_DIR=/opt/homebrew/opt/llvm@17/lib/cmake/llvm
fi

# Build tests
echo -e "${YELLOW}Building tests...${NC}"
cd "$BUILD_DIR"
make -j4 meadows_tests 2>&1 | tail -5

# Check if test executable exists
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
