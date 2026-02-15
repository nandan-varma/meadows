# Security Test Suite for Meadows Compiler

## Overview
This directory contains security-focused tests to ensure the Meadows compiler
is robust against malicious inputs, injection attacks, and resource exhaustion.

## Test Categories

### Overflow Tests (`overflow/`)
- Integer overflow detection
- Buffer overflow prevention
- Stack overflow protection
- Recursion depth limits

### Injection Tests (`injection/`)
- Path traversal attempts
- Command injection
- Format string attacks
- Code injection via imports

### Resource Tests (`resources/`)
- Memory exhaustion handling
- File descriptor leak detection
- CPU usage limits
- Large input handling

## Running Security Tests

```bash
# Run all security tests
./test.sh security

# Run specific security category
./build/tests/meadows_tests "[security]"

# Run overflow tests only
./build/tests/meadows_tests "[security][overflow]"
```

## Security Principles

1. **Fail Secure**: Compiler should reject malicious input safely
2. **Input Validation**: All inputs are validated before processing
3. **Resource Limits**: Memory, CPU, and file limits are enforced
4. **No Code Execution**: Untrusted input cannot execute arbitrary code

## Adding Security Tests

When adding new security tests:
1. Document the attack vector being tested
2. Verify the test fails before the fix
3. Verify the test passes after the fix
4. Include both positive and negative test cases
