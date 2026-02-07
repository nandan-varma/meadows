#!/bin/bash

# Security Fuzzing Tests for Meadows Compiler
# Tests edge cases and malicious inputs

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_ROOT/build/bin/meadows"
TEST_DIR="$PROJECT_ROOT/tests/security"
RESULTS_FILE="$TEST_DIR/fuzz_results.txt"

echo "=== Meadows Compiler Security Fuzzing Tests ===" > "$RESULTS_FILE"
echo "Date: $(date)" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

PASS=0
FAIL=0

log_pass() {
    echo -e "[PASS] $1"
    echo "[PASS] $1" >> "$RESULTS_FILE"
    ((PASS++))
}

log_fail() {
    echo -e "[FAIL] $1"
    echo "[FAIL] $1" >> "$RESULTS_FILE"
    ((FAIL++))
}

log_info() {
    echo -e "[INFO] $1"
    echo "[INFO] $1" >> "$RESULTS_FILE"
}

# Create temp directory
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

echo ""
echo "Running security fuzzing tests..."
echo ""

# Test 1: Empty input
log_info "Test 1: Empty input"
echo "" > "$TEMP_DIR/empty.ms"
if $COMPILER "$TEMP_DIR/empty.ms" 2>&1 | grep -qE "(Error|error)"; then
    log_pass "Empty file handled correctly"
else
    log_fail "Empty file not handled"
fi

# Test 2: Only whitespace
log_info "Test 2: Whitespace only"
echo "   \n\t  " > "$TEMP_DIR/whitespace.ms"
if $COMPILER "$TEMP_DIR/whitespace.ms" 2>&1 | grep -qE "(Error|error)"; then
    log_pass "Whitespace only handled correctly"
else
    log_fail "Whitespace only not handled"
fi

# Test 3: Extremely long identifiers
log_info "Test 3: Long identifiers"
cat > "$TEMP_DIR/long_id.ms" << 'INPUT'
let very_long_identifier_name_that_goes_on_for_a_very_long_time = 42;
print very_long_identifier_name_that_goes_on_for_a_very_long_time;
INPUT
if $COMPILER "$TEMP_DIR/long_id.ms" > /dev/null 2>&1; then
    log_pass "Long identifier handled"
else
    log_fail "Long identifier failed"
fi

# Test 4: Deep nesting
log_info "Test 4: Deep parentheses nesting"
echo "((((((((((((((((((((((((x))))))))))))))))))))))))" > "$TEMP_DIR/deep.ms"
if $COMPILER "$TEMP_DIR/deep.ms" 2>&1 | grep -qE "(Error|error|stack)"; then
    log_pass "Deep nesting detected"
else
    log_info "Deep nesting allowed (check for stack overflow)"
fi

# Test 5: Very long string
log_info "Test 5: Very long string literal"
python3 -c "print('\"' + 'a' * 10000 + '\"')" > "$TEMP_DIR/long_string.ms"
if $COMPILER "$TEMP_DIR/long_string.ms" > /dev/null 2>&1; then
    log_pass "Long string handled"
else
    log_fail "Long string failed"
fi

# Test 6: Many semicolons
log_info "Test 6: Many semicolons"
python3 -c "print('let x = 1' + ';' * 1000)" > "$TEMP_DIR/many_semi.ms"
if $COMPILER "$TEMP_DIR/many_semi.ms" > /dev/null 2>&1; then
    log_pass "Many semicolons handled"
else
    log_fail "Many semicolons failed"
fi

# Test 7: Deep block nesting
log_info "Test 7: Deep block nesting"
python3 -c "print('if (1) { ' * 100 + 'print 1; ' + '}' * 100)" > "$TEMP_DIR/deep_blocks.ms"
if $COMPILER "$TEMP_DIR/deep_blocks.ms" > /dev/null 2>&1; then
    log_pass "Deep blocks handled"
else
    log_fail "Deep blocks failed"
fi

# Test 8: Many function parameters
log_info "Test 8: Many function parameters"
python3 -c "print('func f(' + ', '.join(['x' + str(i) for i in range(100)]) + ') { return 1; }')" > "$TEMP_DIR/many_params.ms"
if $COMPILER "$TEMP_DIR/many_params.ms" > /dev/null 2>&1; then
    log_pass "Many parameters handled"
else
    log_fail "Many parameters failed"
fi

# Test 9: Large numbers
log_info "Test 9: Large numeric literals"
cat > "$TEMP_DIR/large_num.ms" << 'INPUT'
let x = 99999999999999999999999999999999999999999;
print x;
INPUT
if $COMPILER "$TEMP_DIR/large_num.ms" > /dev/null 2>&1; then
    log_pass "Large number handled"
else
    log_fail "Large number failed"
fi

# Test 10: Unicode in identifiers
log_info "Test 10: Unicode characters"
cat > "$TEMP_DIR/unicode.ms" << 'INPUT'
let café = 42;
print café;
INPUT
if $COMPILER "$TEMP_DIR/unicode.ms" 2>&1 | grep -qE "(Error|error)"; then
    log_pass "Unicode rejected (expected)"
else
    log_info "Unicode allowed (check if intentional)"
fi

# Test 11: Null byte in string
log_info "Test 11: Null byte handling"
printf '"hello\x00world"' > "$TEMP_DIR/null_byte.ms"
if $COMPILER "$TEMP_DIR/null_byte.ms" 2>&1 | grep -qE "(Error|error)"; then
    log_pass "Null byte rejected"
else
    log_info "Null byte allowed (check if intentional)"
fi

# Test 12: Recursive function
log_info "Test 12: Deep recursion potential"
cat > "$TEMP_DIR/recursive.ms" << 'INPUT'
func deep(n) {
    if (n > 0) {
        return deep(n - 1);
    }
    return 0;
}
deep(1000);
INPUT
if $COMPILER "$TEMP_DIR/recursive.ms" > /dev/null 2>&1; then
    log_pass "Recursion handled"
else
    log_fail "Recursion failed"
fi

# Test 13: Memory exhaustion attempt
log_info "Test 13: Large array literal"
python3 -c "print('let arr = [' + ','.join([str(i) for i in range(10000)]) + '];')" > "$TEMP_DIR/large_array.ms"
if $COMPILER "$TEMP_DIR/large_array.ms" > /dev/null 2>&1; then
    log_pass "Large array handled"
else
    log_fail "Large array failed"
fi

# Test 14: Cyclic object (if objects supported)
log_info "Test 14: Complex expression nesting"
python3 -c "expr = '1'; [expr := expr + ' + ' + str(i) for i in range(50)]; print('let x = ' + expr + ';')" > "$TEMP_DIR/complex.ms" 2>/dev/null || echo "let x = 1 + 2 + 3 + 4 + 5;" > "$TEMP_DIR/complex.ms"
if $COMPILER "$TEMP_DIR/complex.ms" > /dev/null 2>&1; then
    log_pass "Complex expression handled"
else
    log_fail "Complex expression failed"
fi

# Test 15: Invalid UTF-8
log_info "Test 15: Invalid UTF-8 sequence"
printf '\xff\xfe' > "$TEMP_DIR/invalid_utf8.ms"
if $COMPILER "$TEMP_DIR/invalid_utf8.ms" 2>&1 | grep -qE "(Error|error)"; then
    log_pass "Invalid UTF-8 rejected"
else
    log_info "Invalid UTF-8 allowed (check if intentional)"
fi

# Test 16: Very long line
log_info "Test 16: Very long line"
python3 -c "print('let x = ' + '1+' * 10000 + '1;')" > "$TEMP_DIR/long_line.ms"
if timeout 5 $COMPILER "$TEMP_DIR/long_line.ms" > /dev/null 2>&1; then
    log_pass "Long line handled"
else
    log_fail "Long line failed or timed out"
fi

# Test 17: Nested comments
log_info "Test 17: Comment handling"
cat > "$TEMP_DIR/comments.ms" << 'INPUT'
# This is a comment
// Another comment
let x = 42; # inline comment
print x;
INPUT
if $COMPILER "$TEMP_DIR/comments.ms" > /dev/null 2>&1; then
    log_pass "Comments handled"
else
    log_fail "Comments failed"
fi

# Test 18: All keywords
log_info "Test 18: All keywords in sequence"
cat > "$TEMP_DIR/keywords.ms" << 'INPUT'
let a = 1;
func b(c) { return c; }
if (a) { print a; } else { print 0; }
for (i in range(0, 1)) { print i; }
while (false) { print 0; }
return 0;
INPUT
if $COMPILER "$TEMP_DIR/keywords.ms" > /dev/null 2>&1; then
    log_pass "All keywords handled"
else
    log_fail "Keywords failed"
fi

# Summary
echo ""
echo "=== Fuzzing Test Summary ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
echo "" >> "$RESULTS_FILE"
echo "=== Summary ===" >> "$RESULTS_FILE"
echo "Passed: $PASS" >> "$RESULTS_FILE"
echo "Failed: $FAIL" >> "$RESULTS_FILE" >> "$RESULTS_FILE"

echo ""
echo "Results saved to: $RESULTS_FILE"

if [ $FAIL -gt 0 ]; then
    exit 1
fi

exit 0
