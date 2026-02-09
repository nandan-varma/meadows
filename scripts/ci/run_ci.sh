#!/bin/bash
# CI Entry Point Script
# Usage: ./scripts/ci/run_ci.sh [--quick] [--llvm DIR]
#
# This script is designed to be used both:
# 1. Locally for development testing
# 2. In GitHub Actions CI for automated testing
#
# Options:
#   --quick    Skip integration/security tests (faster)
#   --llvm DIR Use specific LLVM directory

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Parse arguments
QUICK=0
LLVM_DIR=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --quick)
            QUICK=1
            shift
            ;;
        --llvm)
            LLVM_DIR="$2"
            export LLVM_DIR
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows CI Pipeline${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Detect LLVM if not provided
if [[ -z "$LLVM_DIR" ]]; then
    source "$SCRIPT_DIR/../dev/detect_llvm.sh"
    LLVM_DIR=$(get_llvm_dir)
fi

if [[ -z "$LLVM_DIR" ]]; then
    echo -e "${RED}Error: LLVM 17 not found${NC}"
    echo "Install LLVM 17 and try again, or use --llvm to specify location"
    exit 1
fi

echo -e "${YELLOW}Using LLVM: $LLVM_DIR${NC}"
echo

# Detect platform
PLATFORM=$(uname)
echo -e "${YELLOW}Platform: $PLATFORM${NC}"
echo

# Set environment for binary
if [[ "$PLATFORM" == "Linux" ]]; then
    RUNTIME_DIR=$(get_llvm_runtime_dir)
    if [[ -n "$RUNTIME_DIR" ]]; then
        export LD_LIBRARY_PATH="$RUNTIME_DIR:$LD_LIBRARY_PATH"
        echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    fi
fi

NJOBS=$(get_cmake_jobs)
echo "Jobs: $NJOBS"
echo

# Build Release (for packaging)
echo -e "${YELLOW}Building Release...${NC}"
cmake -B build-release \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_DIR="$LLVM_DIR" \
    -DBUILD_TESTS=ON
cmake --build build-release --parallel "$NJOBS"

# Copy release binary to common location for tests
mkdir -p build
if [[ "$PLATFORM" == "Linux" ]] || [[ "$PLATFORM" == "Darwin" ]]; then
    cp build-release/bin/Meadows build/Meadows
elif [[ "$PLATFORM" == MINGW* ]] || [[ "$PLATFORM" == MSYS* ]]; then
    cp build-release/bin/Meadows.exe build/Meadows.exe
fi

echo
echo -e "${YELLOW}Running unit tests...${NC}"
if [[ "$PLATFORM" == "Linux" ]] || [[ "$PLATFORM" == "Darwin" ]]; then
    ./build-release/tests/meadows_tests
elif [[ "$PLATFORM" == MINGW* ]] || [[ "$PLATFORM" == MSYS* ]]; then
    ./build-release/tests/meadows_tests.exe
fi

if [[ $QUICK -eq 0 ]]; then
    echo
    echo -e "${YELLOW}Running integration tests...${NC}"
    ./scripts/test/run_integration_tests.sh

    echo
    echo -e "${YELLOW}Running security tests...${NC}"
    ./scripts/test/run_security_tests.sh
fi

echo
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}   CI Pipeline Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
