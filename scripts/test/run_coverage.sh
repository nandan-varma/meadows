#!/bin/bash
# Run tests with code coverage reporting

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-coverage"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows Coverage Report${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Check for required tools
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}Error: lcov not found. Install with: brew install lcov${NC}"
    exit 1
fi

# Clean previous coverage data
rm -rf "$BUILD_DIR"
rm -f "$PROJECT_ROOT/coverage.info"
rm -rf "$PROJECT_ROOT/coverage_report"

# Auto-detect LLVM
if [[ -z "${LLVM_DIR}" ]]; then
  if [[ "$(uname)" == "Darwin" ]]; then
    if command -v brew >/dev/null 2>&1; then
      if brew list llvm@17 >/dev/null 2>&1; then
        LLVM_DIR="$(brew --prefix llvm@17)/lib/cmake/llvm"
      fi
    fi
  fi
fi

# Build with coverage
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${YELLOW}Building with coverage...${NC}"
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DENABLE_COVERAGE=ON \
  ${LLVM_DIR:+-DLLVM_DIR="${LLVM_DIR}"}

make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc) meadows_tests

# Run tests
echo
echo -e "${YELLOW}Running tests...${NC}"
"$BUILD_DIR/tests/meadows_tests"

# Generate coverage report
echo
echo -e "${YELLOW}Generating coverage report...${NC}"
cd "$PROJECT_ROOT"

lcov --capture --directory "$BUILD_DIR" --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info '*/catch_amalgamated*' --output-file coverage.info
lcov --remove coverage.info '*/tests/*' --output-file coverage.info

# Generate HTML report
genhtml coverage.info --output-directory coverage_report

# Display summary
echo
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}   Coverage report generated!${NC}"
echo -e "${BLUE}========================================${NC}"
echo
echo "View report: coverage_report/index.html"
echo

# Show line coverage percentage
if command -v lcov &> /dev/null; then
    lcov --summary coverage.info 2>&1 | grep "lines" | head -1
fi
