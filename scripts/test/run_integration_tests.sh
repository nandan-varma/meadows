#!/bin/bash
# Run integration tests for all .ms files in examples/ and tests/integration/.
#
# Expected-output lookup:
#   examples/foo.ms        → examples/foo.ms.expected     (sidecar)
#   tests/integration/foo.ms → tests/integration/expected/foo.expected (subdirectory)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

VERBOSE=0
NO_BUILD=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose) VERBOSE=1; shift ;;
        --no-build)   NO_BUILD=1; shift ;;
        *) shift ;;
    esac
done

# ── Locate compiler ───────────────────────────────────────────────────────────

COMPILER=""
for dir in \
    "$PROJECT_ROOT/build/bin" \
    "$PROJECT_ROOT/build" \
    "$PROJECT_ROOT/build-debug/bin" \
    "$PROJECT_ROOT/build-release/bin"; do
    if [[ -f "$dir/Meadows" ]]; then
        COMPILER="$dir/Meadows"
        break
    fi
done

if [[ -z "$COMPILER" ]]; then
    COMPILER=$(find "$PROJECT_ROOT" -name "Meadows" -type f -executable 2>/dev/null | head -1)
fi

if [[ -z "$COMPILER" ]]; then
    if [[ $NO_BUILD -eq 0 ]]; then
        echo -e "${YELLOW}Compiler not found — building…${NC}"
        "$PROJECT_ROOT/build.sh" debug
        COMPILER="$PROJECT_ROOT/build-debug/bin/Meadows"
    else
        echo -e "${RED}Error: Compiler not found. Run ./build.sh first.${NC}"
        exit 1
    fi
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows Integration Tests${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Compiler: $COMPILER"
echo

# ── Test runner ───────────────────────────────────────────────────────────────
# run_tests_in_dir <ms_dir> [expected_dir]
#   ms_dir       directory containing *.ms files
#   expected_dir if set, expected files live here as <stem>.expected;
#                if unset, expected files live alongside as <file>.ms.expected

TOTAL=0
PASSED=0
FAILED=0

run_tests_in_dir() {
    local ms_dir="$1"
    local expected_dir="${2:-}"

    shopt -s nullglob
    local files=("$ms_dir"/*.ms)
    shopt -u nullglob

    [[ ${#files[@]} -eq 0 ]] && return

    for ms_file in "${files[@]}"; do
        local stem
        stem=$(basename "${ms_file%.ms}")

        local expected_file
        if [[ -n "$expected_dir" ]]; then
            expected_file="$expected_dir/${stem}.expected"
        else
            expected_file="${ms_file}.expected"
        fi

        [[ -f "$expected_file" ]] || continue

        TOTAL=$((TOTAL + 1))

        # Compile
        if ! "$COMPILER" "$ms_file" -o "${ms_file%.ms}.out" > /dev/null 2>&1; then
            echo -e "${RED}✗${NC} ${stem} (compilation failed)"
            FAILED=$((FAILED + 1))
            continue
        fi

        # Run
        local actual
        actual=$("${ms_file%.ms}.out" 2>&1)
        local expected
        expected=$(cat "$expected_file")

        # Clean up
        rm -f "${ms_file%.ms}.out" "${ms_file}.ll"

        if [[ "$actual" == "$expected" ]]; then
            [[ $VERBOSE -eq 1 ]] && echo -e "${GREEN}✓${NC} ${stem}"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}✗${NC} ${stem}"
            if [[ $VERBOSE -eq 1 ]]; then
                echo "  expected: $(echo "$expected" | head -3)"
                echo "  got:      $(echo "$actual"   | head -3)"
            fi
            FAILED=$((FAILED + 1))
        fi
    done
}

# run_error_tests_in_dir <dir>
# Each .ms file is expected to FAIL compilation (exit != 0).
# If a <stem>.expected_err sidecar exists, its content must appear in stderr.

run_error_tests_in_dir() {
    local dir="$1"

    shopt -s nullglob
    local files=("$dir"/*.ms)
    shopt -u nullglob

    [[ ${#files[@]} -eq 0 ]] && return

    for ms_file in "${files[@]}"; do
        local stem
        stem=$(basename "${ms_file%.ms}")

        TOTAL=$((TOTAL + 1))

        local stderr_out
        stderr_out=$("$COMPILER" "$ms_file" 2>&1 >/dev/null)
        local exit_code=$?

        # Clean up any partial output
        rm -f "${ms_file%.ms}.out" "${ms_file%.ms}.ll"

        if [[ $exit_code -eq 0 ]]; then
            echo -e "${RED}✗${NC} ${stem} (expected compilation failure, but it succeeded)"
            FAILED=$((FAILED + 1))
            continue
        fi

        # If a .expected_err sidecar exists, check that stderr contains it
        local expected_err_file="${ms_file%.ms}.expected_err"
        if [[ -f "$expected_err_file" ]]; then
            local expected_substr
            expected_substr=$(cat "$expected_err_file")
            if [[ "$stderr_out" != *"$expected_substr"* ]]; then
                echo -e "${RED}✗${NC} ${stem} (wrong error: expected '${expected_substr}' in stderr)"
                if [[ $VERBOSE -eq 1 ]]; then
                    echo "  stderr: $(echo "$stderr_out" | head -3)"
                fi
                FAILED=$((FAILED + 1))
                continue
            fi
        fi

        [[ $VERBOSE -eq 1 ]] && echo -e "${GREEN}✓${NC} ${stem} (correctly rejected)"
        PASSED=$((PASSED + 1))
    done
}

run_tests_in_dir "$PROJECT_ROOT/examples"
run_tests_in_dir "$PROJECT_ROOT/tests/integration" "$PROJECT_ROOT/tests/integration/expected"
run_error_tests_in_dir "$PROJECT_ROOT/tests/integration/errors"

# ── Summary ───────────────────────────────────────────────────────────────────

echo
echo -e "${BLUE}========================================${NC}"
if [[ $TOTAL -eq 0 ]]; then
    echo -e "${YELLOW}No integration tests found${NC}"
    exit 1
elif [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}   All $TOTAL tests passed${NC}"
    echo -e "${BLUE}========================================${NC}"
    exit 0
else
    echo -e "${RED}   $PASSED/$TOTAL passed, $FAILED failed${NC}"
    echo -e "${BLUE}========================================${NC}"
    exit 1
fi
