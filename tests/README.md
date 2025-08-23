# Meadows Language Test Suite

This directory contains a comprehensive test suite for the Meadows Python-like programming language compiler. The test suite is designed to validate all aspects of the language implementation, from lexical analysis to code generation.

## Test Structure

### Test Categories

1. **Lexer Tests** (`test_lexer.py`)
   - Token recognition for all language constructs
   - Integer, float, string, identifier, keyword, and operator tokens
   - Edge cases and boundary conditions

2. **Expression Tests** (`test_parser_expressions.py`)
   - Binary and unary expressions
   - Operator precedence and associativity
   - Function calls, attribute access, method chaining
   - Complex nested expressions

3. **Statement Tests** (`test_parser_statements.py`)
   - Assignment statements
   - Control flow (if, while, for)
   - Return statements
   - Block structure and indentation

4. **Function Tests** (`test_functions.py`)
   - Function definitions with parameters
   - Default parameters
   - Recursive functions
   - Complex function bodies

5. **Class Tests** (`test_classes.py`)
   - Class definitions with methods
   - Constructor (`__init__`) methods
   - Instance variables and methods
   - Complex class hierarchies

6. **Precedence Tests** (`test_precedence.py`)
   - Arithmetic operator precedence
   - Comparison operator precedence
   - Logical operator precedence
   - Parentheses grouping

7. **Control Flow Tests** (`test_control_flow.py`)
   - Nested control structures
   - Early returns and breaks
   - Complex boolean conditions
   - Loop edge cases

8. **Data Type Tests** (`test_data_types.py`)
   - All supported literal types
   - Type interactions and mixing
   - Type-specific operations

9. **Edge Case Tests** (`test_edge_cases.py`)
   - Boundary conditions
   - Error scenarios
   - Complex expressions
   - Deep nesting

10. **Comprehensive Test** (`comprehensive_test.py`)
    - Integration test covering all language features
    - Real-world programming examples
    - End-to-end functionality validation

## Running Tests

### Automated Test Runner

Use the provided shell script for easy test execution:

```bash
# Run all tests
./run_tests.sh

# Run specific test categories
./run_tests.sh build      # Build project only
./run_tests.sh test       # Run comprehensive test suite
./run_tests.sh individual # Test individual files
./run_tests.sh examples   # Test example programs
./run_tests.sh compile    # Test compilation to executable
./run_tests.sh compiler   # Run built-in compiler tests
./run_tests.sh clean      # Clean build directory
```

### Manual Testing

You can also run tests manually:

```bash
# Build the project
mkdir -p build && cd build
cmake ..
make

# Run individual test files
./meadows -p ../tests/test_lexer.py
./meadows -p ../tests/test_functions.py

# Run comprehensive test suite
./meadows_test

# Run built-in compiler tests
./meadows
```

## Test Framework

### C++ Test Framework

The project includes a custom C++ test framework (`src/testing/TestFramework.h/cpp`) that provides:

- **Test Case Management**: Organize tests by category and functionality
- **Automated Validation**: Built-in validators for AST structure
- **Error Handling**: Tests for both success and failure scenarios
- **Detailed Reporting**: Comprehensive test results and statistics
- **File Loading**: Automatic loading of test files from the filesystem

### Test Case Structure

Each test case includes:
- **Name**: Unique identifier for the test
- **Description**: Human-readable description of what's being tested
- **Source Code**: The Meadows language code to test
- **Expected Result**: Whether the test should pass or fail
- **Custom Validator**: Optional function to validate specific aspects of the result

### Example Test Case

```cpp
addTest(TestCase(
    "func_recursive",
    "Parse recursive function",
    "def factorial(n):\n    if n <= 1:\n        return 1\n    else:\n        return n * factorial(n - 1)",
    true,  // expect success
    [](const Program& program) {
        // Custom validation logic
        return program.statements.size() > 0;
    }
));
```

## Test Coverage

The test suite covers:

### Language Features
- ✅ Basic data types (int, float, string, bool, None)
- ✅ Arithmetic expressions with proper precedence
- ✅ Comparison and logical operations
- ✅ Variable assignment
- ✅ Function definitions and calls
- ✅ Class definitions with methods
- ✅ Control flow (if/else, while, for)
- ✅ Block structure and indentation
- ✅ Return statements
- ✅ Method chaining and attribute access

### Error Handling
- ✅ Syntax error detection
- ✅ Invalid indentation
- ✅ Missing punctuation
- ✅ Malformed expressions
- ✅ Graceful error recovery

### Edge Cases
- ✅ Empty programs
- ✅ Comment-only files
- ✅ Deep nesting
- ✅ Complex expressions
- ✅ Boundary value testing

## Expected Results

### Successful Test Run

A successful test run should show:
- All lexer tests passing (token recognition)
- All parser tests passing (syntax analysis)
- All AST tests passing (tree construction)
- Integration tests passing (complete programs)
- Error tests correctly failing for invalid input

### Sample Output

```
=== Meadows Language Test Suite ===
Running 45 tests...

Running test: lex_integers - Tokenize integer literals
  PASS

Running test: func_recursive - Parse recursive function
  PASS

...

=== Test Results ===
Total tests: 45
Passed: 42
Failed: 0
Errors: 3
🎉 All critical tests passed!
```

## Adding New Tests

### Creating Test Files

1. Create new `.py` files in the `tests/` directory
2. Use valid Meadows language syntax
3. Include comments describing what's being tested
4. Cover both positive and negative test cases

### Adding to Test Framework

1. Add new test cases to `TestFramework.cpp`
2. Use appropriate categories (lexer, parser, etc.)
3. Include custom validators when needed
4. Update the test runner script if necessary

### Test File Naming Convention

- `test_<category>.py` for category-specific tests
- Use descriptive function names within test files
- Include edge cases and error conditions

## Continuous Integration

The test suite is designed to be run in CI/CD environments:

- Exit codes indicate test success/failure
- Detailed logging for debugging
- Modular execution for specific test categories
- Cross-platform compatibility (macOS, Linux, Windows)

## Performance Testing

While not included in this initial suite, consider adding:

- Large file parsing tests
- Memory usage validation
- Compilation time benchmarks
- Deep recursion limits

## Future Enhancements

Planned additions to the test suite:

1. **Semantic Analysis Tests**: Once type checking is implemented
2. **Code Generation Tests**: Validation of generated LLVM IR
3. **Runtime Tests**: Testing compiled executable behavior
4. **Standard Library Tests**: When built-in functions are added
5. **Performance Benchmarks**: Compilation and execution speed tests
6. **Fuzzing Tests**: Automated generation of test cases
