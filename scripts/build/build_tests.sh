#!/bin/bash
# Build Test executables for Meadows Compiler

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Building Tests...${NC}"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "  Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_DIR=/opt/homebrew/opt/llvm@17/lib/cmake/llvm \
    -DBUILD_TESTS=ON

# Build
echo "  Compiling tests..."
make -j4 meadows_tests 2>&1 | tail -10

if [ ! -f "$BUILD_DIR/tests/meadows_tests" ]; then
    echo -e "${RED}Error: Test build failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Tests built successfully${NC}"
echo -e "${GREEN}Test executable: $BUILD_DIR/tests/meadows_tests${NC}"
