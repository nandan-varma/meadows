# Integration Test Suite for Meadows Compiler

## Overview
This directory contains comprehensive integration tests that verify end-to-end
behavior of the Meadows compiler, including multi-file projects, error handling,
and edge cases.

## Test Categories

### 1. Multi-file Module Tests (`modules/`)
Tests for the module system including imports, exports, and cross-module dependencies.

### 2. Error Handling Tests (`errors/`)
Tests that verify error messages, recovery, and diagnostics.

### 3. Edge Cases (`edge_cases/`)
Stress tests for boundary conditions and unusual inputs.

### 4. Full Programs (`*.ms` files)
Complete programs that exercise multiple compiler features.

## Running Integration Tests

```bash
# Run all integration tests
./test.sh integration

# Run specific category
./build/tests/meadows_tests "[integration]"

# Run with output
./build/tests/meadows_tests "[integration]" -s
```

## Test Format

Each `.ms` file should be a valid Meadows program. Test expectations are
checked by the test runner which compiles and optionally executes the program.

### Expected Output Files
For tests with expected output, create a `.expected` file with the same basename:
- `test.ms` → `test.expected`

### Expected Error Files
For tests that should fail, create a `.error` file:
- `error_test.ms` → `error_test.error` (contains expected error message)

## Adding New Tests

1. Create `.ms` file in appropriate directory
2. Add expected output if applicable
3. Document test purpose in this README
4. Run `./test.sh integration` to verify

## Coverage Goals

- Module system: 95%
- Error handling: 90%
- Edge cases: 100% of identified boundaries
- Full programs: All major language features
