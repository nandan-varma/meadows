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

# Run unit tests directly via the Catch2 binary.
run_unit_tests() {
    local test_bin="$SCRIPT_DIR/build/tests/meadows_tests"
    echo -e "${YELLOW}Running Unit tests...${NC}"

    if [[ ! -f "$test_bin" ]]; then
        echo -e "${RED}Error: test executable not found: $test_bin${NC}"
        echo "Run: ./build.sh tests"
        return 1
    fi

    local catch_args=(--reporter console)
    [[ -n "$VERBOSE" ]] && catch_args+=(--verbosity high)
    [[ -n "$FAIL_FAST" ]] && catch_args+=(-x)

    if "$test_bin" "${catch_args[@]}"; then
        echo -e "${GREEN}Unit tests passed${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}Unit tests failed${NC}"
        FAILED=$((FAILED + 1))
        [[ -n "$FAIL_FAST" ]] && return 1
    fi
    echo
}

# Run an external test suite script (integration, security, etc.).
run_suite_script() {
    local suite=$1 name="$2"
    local script="$SCRIPT_DIR/scripts/test/run_${suite}_tests.sh"

    if [[ ! -f "$script" ]]; then
        echo -e "${YELLOW}Warning: Test script not found: $script${NC}"
        echo
        return 0
    fi

    echo -e "${YELLOW}Running ${name}...${NC}"
    if "$script" $VERBOSE $FAIL_FAST 2>/dev/null; then
        echo -e "${GREEN}${name} passed${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}${name} failed${NC}"
        FAILED=$((FAILED + 1))
        [[ -n "$FAIL_FAST" ]] && return 1
    fi
    echo
}

case $SUITE in
    all)
        run_unit_tests
        run_suite_script "integration" "Integration tests"
        run_suite_script "security"    "Security tests"
        ;;
    unit)
        run_unit_tests
        ;;
    integration)
        run_suite_script "integration" "Integration tests"
        ;;
    security)
        run_suite_script "security" "Security tests"
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
