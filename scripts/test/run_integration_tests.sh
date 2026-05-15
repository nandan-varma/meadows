#!/bin/bash
# Run integration tests (.ms files)

# Don't exit on error - we handle failures ourselves
# set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
COMPILER=""

# Find compiler in order of preference
for dir in "$BUILD_DIR/bin" "$BUILD_DIR" "$PROJECT_ROOT/build-debug/bin" "$PROJECT_ROOT/build-release/bin"; do
    if [ -f "$dir/Meadows" ]; then
        COMPILER="$dir/Meadows"
        break
    fi
done

# Fallback to any Meadows binary
if [ -z "$COMPILER" ]; then
    COMPILER=$(find "$PROJECT_ROOT" -name "Meadows" -type f -executable 2>/dev/null | head -1)
fi

if [ -z "$COMPILER" ] || [ ! -f "$COMPILER" ]; then
    echo -e "${RED}Error: Compiler not found${NC}"
    echo "Searched in:"
    echo "  $BUILD_DIR/bin/Meadows"
    echo "  $BUILD_DIR/Meadows"
    echo "  $PROJECT_ROOT/build-debug/bin/Meadows"
    echo "  $PROJECT_ROOT/build-release/bin/Meadows"
    exit 1
fi

echo -e "${YELLOW}Using compiler: $COMPILER${NC}"

TEST_DIR="$PROJECT_ROOT/examples"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Parse arguments
VERBOSE=0
NO_BUILD=0

while [[ $# -gt 0 ]]; do
  case $1 in
    -v|--verbose)
      VERBOSE=1
      shift
      ;;
    --no-build)
      NO_BUILD=1
      shift
      ;;
    *)
      shift
      ;;
  esac
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows Integration Tests${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Build if needed
if [ $NO_BUILD -eq 0 ]; then
    if [ ! -f "$COMPILER" ]; then
        echo -e "${YELLOW}Compiler not found. Building...${NC}"
        "$PROJECT_ROOT/build.sh" debug
    fi
fi

# Clean up any existing generated files
rm -f "$TEST_DIR"/*.ll "$TEST_DIR"/*.out

total_tests=0
passed=0

for file in "$TEST_DIR"/*.ms; do
    expected_file="${file}.expected"
    if [ ! -f "$expected_file" ]; then
        continue
    fi

    ((total_tests++))
    test_name=$(basename "$file")

    if [ $VERBOSE -eq 1 ]; then
        echo -n "Testing $test_name: "
    fi

    # Compile the .ms file
    if ! "$COMPILER" "$file" > /dev/null 2>&1; then
        if [ $VERBOSE -eq 1 ]; then
            echo -e "${RED}COMPILATION FAILED${NC}"
        else
            echo -e "${RED}FAIL${NC} $test_name (compilation)"
        fi
        continue
    fi

    # Run the compiled executable and capture output
    output=$("${file}.out" 2>&1)
    expected=$(cat "$expected_file")

    if [ "$output" == "$expected" ]; then
        if [ $VERBOSE -eq 1 ]; then
            echo -e "${GREEN}PASS${NC}"
        else
            echo -e "${GREEN}✓${NC} $test_name"
        fi
        ((passed++))
    else
        if [ $VERBOSE -eq 1 ]; then
            echo -e "${RED}FAIL${NC}"
            echo "  Expected: '$expected'"
            echo "  Got:      '$output'"
        else
            echo -e "${RED}✗${NC} $test_name"
        fi
    fi

    # Clean up generated files
    rm -f "${file}.ll" "${file}.out"
done

if [ $total_tests -eq 0 ]; then
    echo -e "${YELLOW}No test cases found (missing .expected files)${NC}"
    exit 1
fi

percentage=$((passed * 100 / total_tests))

echo
echo -e "${BLUE}========================================${NC}"
if [ $passed -eq $total_tests ]; then
    echo -e "${GREEN}   All $total_tests tests passed (100%)${NC}"
else
    echo -e "${RED}   $passed/$total_tests tests passed ($percentage%)${NC}"
fi
echo -e "${BLUE}========================================${NC}"

if [ $passed -eq $total_tests ]; then
    exit 0
else
    exit 1
fi
