#!/bin/bash

# Security Test Suite for Meadows Compiler
# Tests for Batch 1: Critical Security Fixes

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_ROOT/build/bin/Meadows"
TEST_DIR="$PROJECT_ROOT/tests/security"

# Check for compiler in multiple locations
if [ ! -f "$COMPILER" ]; then
    COMPILER="$PROJECT_ROOT/build/Meadows"
fi
if [ ! -f "$COMPILER" ]; then
    COMPILER="$PROJECT_ROOT/build-debug/Meadows"
fi
if [ ! -f "$COMPILER" ]; then
    COMPILER="$PROJECT_ROOT/build-release/Meadows"
fi
PASSED=0
FAILED=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED++))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED++))
}

# Test 1: Command Injection Prevention
test_command_injection() {
    log_info "Test 1: Command Injection Prevention"
    
    # Create malicious filename that attempts command injection
    local malicious_file="$TEST_DIR/test; rm -rf /.ms"
    echo 'print "test";' > "$malicious_file"
    
    if $COMPILER "$malicious_file" 2>&1 | grep -qE "(Invalid characters|Error:|invalid)"; then
        log_pass "Command injection attempt blocked: semicolon in filename"
    else
        log_fail "Command injection NOT blocked for semicolon"
    fi
    
    rm -f "$malicious_file"
    
    # Test with pipe character
    malicious_file="$TEST_DIR/test|whoami.ms"
    echo 'print "test";' > "$malicious_file"
    
    if $COMPILER "$malicious_file" 2>&1 | grep -qE "(Invalid characters|Error:|invalid)"; then
        log_pass "Command injection attempt blocked: pipe in filename"
    else
        log_fail "Command injection NOT blocked for pipe"
    fi
    
    rm -f "$malicious_file"
    
    # Test with backtick
    malicious_file="$TEST_DIR/test\`whoami\`.ms"
    echo 'print "test";' > "$malicious_file"
    
    if $COMPILER "$malicious_file" 2>&1 | grep -qE "(Invalid characters|Error:|invalid)"; then
        log_pass "Command injection attempt blocked: backtick in filename"
    else
        log_fail "Command injection NOT blocked for backtick"
    fi
    
    rm -f "$malicious_file"
}

# Test 2: Path Traversal Prevention
test_path_traversal() {
    log_info "Test 2: Path Traversal Prevention"
    
    # Test parent directory traversal
    local traversal_file="../../../etc/passwd.ms"
    
    if $COMPILER "$traversal_file" 2>&1 | grep -qE "(must be in current directory|Invalid file path|Error:)"; then
        log_pass "Path traversal blocked: ../../../etc/passwd.ms"
    else
        log_fail "Path traversal NOT blocked"
    fi
    
    # Test current directory file (should work)
    local valid_file="$TEST_DIR/valid_test.ms"
    echo 'print "hello";' > "$valid_file"
    
    if $COMPILER "$valid_file" > /dev/null 2>&1; then
        log_pass "Valid file in current directory accepted"
        rm -f "$valid_file" "$valid_file.ll" "$valid_file.out"
    else
        log_fail "Valid file rejected"
        rm -f "$valid_file"
    fi
}

# Test 3: File Extension Validation
test_extension_validation() {
    log_info "Test 3: File Extension Validation"
    
    # Test file without .ms extension
    local wrong_ext="$TEST_DIR/test.txt"
    echo 'print "test";' > "$wrong_ext"
    
    if $COMPILER "$wrong_ext" 2>&1 | grep -qE "(must have .ms extension|Error:|invalid)"; then
        log_pass "Non-.ms extension rejected"
    else
        log_fail "Non-.ms extension accepted (should be rejected)"
    fi
    
    rm -f "$wrong_ext"
    
    # Test file with .ms extension (should work)
    local correct_ext="$TEST_DIR/test.ms"
    echo 'print "test";' > "$correct_ext"
    
    if $COMPILER "$correct_ext" > /dev/null 2>&1; then
        log_pass ".ms extension accepted"
        rm -f "$correct_ext" "$correct_ext.ll" "$correct_ext.out"
    else
        log_fail ".ms extension rejected (should be accepted)"
        rm -f "$correct_ext"
    fi
}

# Test 4: File Size Limit
test_file_size_limit() {
    log_info "Test 4: File Size Limit"
    
    # Create a file larger than 10MB
    local large_file="$TEST_DIR/large_test.ms"
    
    # Generate a large file (11MB)
    python3 -c "
with open('$large_file', 'w') as f:
    # Write 11MB of valid Meadows code
    for i in range(1150000):
        f.write('let x = 1;\n')
    f.write('print 1;\n')
" 2>/dev/null || dd if=/dev/zero of="$large_file" bs=1024 count=11264 2>/dev/null
    
    if [ -f "$large_file" ]; then
        if $COMPILER "$large_file" 2>&1 | grep -qE "(File too large|Error:)"; then
            log_pass "Large file (>10MB) rejected"
        else
            log_fail "Large file accepted (should be rejected)"
        fi
        rm -f "$large_file"
    else
        log_info "Skipping file size test (could not create large file)"
    fi
}

# Test 5: Unterminated String Error
test_unterminated_string() {
    log_info "Test 5: Unterminated String Error Handling"
    
    local test_file="$TEST_DIR/unterminated.ms"
    echo 'print "hello;' > "$test_file"
    
    if $COMPILER "$test_file" 2>&1 | grep -qiE "(unterminated|Lexical error|error)"; then
        log_pass "Unterminated string produces error"
    else
        log_fail "Unterminated string does not produce error"
    fi
    
    rm -f "$test_file" "$test_file.ll" "$test_file.out"
}

# Test 6: Undefined Variable Error
test_undefined_variable() {
    log_info "Test 6: Undefined Variable Error Handling"
    
    local test_file="$TEST_DIR/undefined_var.ms"
    echo 'print x;' > "$test_file"
    
    if $COMPILER "$test_file" 2>&1 | grep -qiE "(undefined|Code generation error|error|Undefined variable)"; then
        log_pass "Undefined variable produces error"
    else
        log_fail "Undefined variable does not produce error"
    fi
    
    rm -f "$test_file" "$test_file.ll" "$test_file.out"
}

# Test 7: Undefined Function Error
test_undefined_function() {
    log_info "Test 7: Undefined Function Error Handling"
    
    local test_file="$TEST_DIR/undefined_func.ms"
    echo 'let x = nonexistent(1);' > "$test_file"
    
    if $COMPILER "$test_file" 2>&1 | grep -qiE "(undefined|Code generation error|error|Undefined function)"; then
        log_pass "Undefined function produces error"
    else
        log_fail "Undefined function does not produce error"
    fi
    
    rm -f "$test_file" "$test_file.ll" "$test_file.out"
}

# Test 8: Syntax Error Handling
test_syntax_error() {
    log_info "Test 8: Syntax Error Handling"
    
    local test_file="$TEST_DIR/syntax_error.ms"
    echo 'let x =' > "$test_file"
    
    if $COMPILER "$test_file" 2>&1 | grep -qiE "(Parse error|syntax|error|Expect)"; then
        log_pass "Syntax error produces parse error"
    else
        log_fail "Syntax error does not produce parse error"
    fi
    
    rm -f "$test_file" "$test_file.ll" "$test_file.out"
}

# Main
main() {
    echo "=========================================="
    echo "Meadows Compiler Security Test Suite"
    echo "=========================================="
    echo ""
    
    # Check if compiler exists
    if [ ! -f "$COMPILER" ]; then
        echo "Error: Compiler not found at $COMPILER"
        echo "Please build the project first with ./build.sh"
        exit 1
    fi
    
    # Run all tests
    test_command_injection
    test_path_traversal
    test_extension_validation
    test_file_size_limit
    test_unterminated_string
    test_undefined_variable
    test_undefined_function
    test_syntax_error
    
    echo ""
    echo "=========================================="
    echo "Results: $PASSED passed, $FAILED failed"
    echo "=========================================="
    
    if [ $FAILED -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        exit 0
    else
        echo -e "${RED}Some tests failed!${NC}"
        exit 1
    fi
}

main "$@"
