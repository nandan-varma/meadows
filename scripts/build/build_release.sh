#!/bin/bash
# Build Release version of Meadows Compiler

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-release"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Detect LLVM
source "$SCRIPT_DIR/../dev/detect_llvm.sh"
LLVM_DIR="${LLVM_DIR:-$(get_llvm_dir)}"
if [[ -z "$LLVM_DIR" ]]; then
    echo -e "${RED}Error: LLVM 17 not found${NC}"
    echo "Install with: brew install llvm@17 (macOS) or apt install llvm-17-dev (Linux)"
    exit 1
fi

echo -e "${BLUE}Building Release version...${NC}"
echo "LLVM_DIR: $LLVM_DIR"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_DIR="$LLVM_DIR" \
    -DBUILD_TESTS=ON

# Build
NJOBS=$(get_cmake_jobs)
echo "Compiling with $NJOBS jobs..."
cmake --build . --parallel "$NJOBS" --target Meadows

if [ ! -f "$BUILD_DIR/bin/Meadows" ]; then
    echo -e "${RED}Error: Release build failed${NC}"
    exit 1
fi

echo -e "${GREEN}Release build complete${NC}"
echo "Binary: $BUILD_DIR/bin/Meadows"
