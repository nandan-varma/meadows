#!/bin/bash
# Meadows Compiler Test Script
# Usage: ./test.sh [SUITE] [OPTIONS]
#
# SUITES:
#   all          Run all tests (default)
#   unit         Run unit tests only
#   integration  Run integration tests (.ms files)
#   security     Run security tests
#   coverage     Run tests with coverage report
#
# OPTIONS:
#   -v           Verbose output
#   -f           Fail fast (stop on first failure)
#   --no-build   Don't rebuild before testing
#
# Examples:
#   ./test.sh                     # Run all tests
#   ./test.sh unit                # Run unit tests only
#   ./test.sh integration -v      # Verbose integration tests
#   ./test.sh coverage            # Generate coverage report

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Default values
SUITE="all"
VERBOSE=""
FAIL_FAST=""
NO_BUILD=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        all|unit|integration|security|coverage)
            SUITE="$1"
            shift
            ;;
        -v|--verbose)
            VERBOSE="-v"
            shift
            ;;
        -f|--fail-fast)
            FAIL_FAST="-f"
            shift
            ;;
        --no-build)
            NO_BUILD=1
            shift
            ;;
        -h|--help)
            sed -n '2,20p' "$0"
            exit 0
            ;;
        *)
            shift
            ;;
    esac
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows Compiler Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Build first if needed
if [ $NO_BUILD -eq 0 ]; then
    echo -e "${YELLOW}Building project...${NC}"
    "$SCRIPT_DIR/build.sh" tests > /dev/null 2>&1 || true
    echo -e "${GREEN}Build complete${NC}"
    echo
fi

# Track results
FAILED=0
PASSED=0

run_test_suite() {
    local suite=$1
    local script="$SCRIPT_DIR/scripts/test/run_${suite}_tests.sh"
    local name="$2"
    
    if [ -f "$script" ]; then
        echo -e "${YELLOW}Running ${name}...${NC}"
        if "$script" $VERBOSE $FAIL_FAST 2>/dev/null; then
            echo -e "${GREEN}${name} passed${NC}"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}${name} failed${NC}"
            FAILED=$((FAILED + 1))
            if [ -n "$FAIL_FAST" ]; then
                return 1
            fi
        fi
        echo
    else
        echo -e "${YELLOW}Warning: Test script not found: $script${NC}"
        echo
    fi
}

case $SUITE in
    all)
        run_test_suite "unit" "Unit tests"
        run_test_suite "integration" "Integration tests"
        run_test_suite "security" "Security tests"
        ;;
    unit)
        run_test_suite "unit" "Unit tests"
        ;;
    integration)
        run_test_suite "integration" "Integration tests"
        ;;
    security)
        run_test_suite "security" "Security tests"
        ;;
    coverage)
        "$SCRIPT_DIR/scripts/test/run_coverage.sh"
        ;;
    *)
        echo -e "${RED}Error: Unknown test suite '$SUITE'${NC}"
        echo "Run './test.sh --help' for usage"
        exit 1
        ;;
esac

echo -e "${BLUE}========================================${NC}"
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}   All test suites passed!${NC}"
    echo -e "${GREEN}   Passed: $PASSED${NC}"
    exit 0
else
    echo -e "${RED}   Some test suites failed!${NC}"
    echo -e "${RED}   Passed: $PASSED, Failed: $FAILED${NC}"
    exit 1
fi
