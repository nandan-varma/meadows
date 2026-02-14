# Meadows Compiler Testing Guide

This document describes the testing infrastructure for the Meadows compiler.

## Test Structure

```
tests/
├── unit/              # Unit tests by module
│   ├── lexer/
│   ├── parser/
│   ├── codegen/
│   ├── ast/
│   └── utils/
├── integration/       # Full program tests (.ms files)
│   ├── factorial.ms
│   ├── hello.ms
│   ├── loop.ms
│   └── variables.ms
└── security/          # Security and fuzz tests
```

## Running Tests

### All Tests
```bash
./test.sh              # Full test suite
./test.sh unit         # Unit tests only
./test.sh integration # Integration tests only
./test.sh security     # Security tests only
```

### Specific Unit Tests
```bash
./build/tests/meadows_tests "[lexer]"       # Run lexer tests
./build/tests/meadows_tests "[parser]"      # Run parser tests
./build/tests/meadows_tests "[exceptions]"  # Run exception tests
./build/tests/meadows_tests "[diagnostics]" # Run diagnostics tests
./build/tests/meadows_tests "[warnings]"    # Run warning tests
./build/tests/meadows_tests "[timer]"      # Run timer tests
./build/tests/meadows_tests "Test Name"    # Run specific test case
```

## Adding Unit Tests

### Test File Structure
Create a new file in `tests/unit/<module>/`:

```cpp
#include "catch_amalgamated.hpp"
#include "<module>/Module.h"

using namespace meadows;

TEST_CASE("ModuleName operation", "[module]") {
    SECTION("Specific scenario") {
        // Test code
        REQUIRE(condition);
    }
}

TEST_CASE("Another operation", "[module][other-tag]") {
    // Test code
}
```

### Test Tags
- Use module tags: `[lexer]`, `[parser]`, `[codegen]`, `[ast]`, `[utils]`
- Use feature tags: `[exceptions]`, `[diagnostics]`, `[warnings]`, `[timer]`
- Use property tags: `[property]` for property-based tests

### Example
```cpp
TEST_CASE("Lexer tokenizes identifiers", "[lexer]") {
    Lexer lexer("let x = 5;");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens.size() == 5);
    REQUIRE(tokens[0].type == TokenType::LET);
    REQUIRE(tokens[1].value == "x");
}
```

## Integration Tests

Add `.ms` files to `tests/integration/`:

```meadows
// Example: factorial.ms
fn factorial(n: int) -> int {
    if n <= 1 {
        return 1;
    }
    return n * factorial(n - 1);
}

let result = factorial(5);
print result;
```

## CLI Command Testing

Commands are located in `examples/cli/commands/`:

```bash
# Build a command
cd examples/cli/commands
/Users/nandan/dev/meadows/build/bin/Meadows ls.ms

# Run the command
./ls.ms.out .

# Test with arguments
./echo.ms.out hello world
# Output: hello world
```

### Available Commands

| Command | Usage | Description |
|---------|-------|-------------|
| `ls.ms` | `./ls.ms.out [dir]` | List directory contents |
| `cat.ms` | `./cat.ms file.txt` | Print file contents |
| `echo.ms` | `./echo.ms arg1 arg2` | Print arguments |
| `head.ms` | `./head.ms -n 5 file.txt` | Print first N lines |
| `tail.ms` | `./tail.ms -n 5 file.txt` | Print last N lines |
| `wc.ms` | `./wc.ms file.txt` | Count lines/words/chars |

### Testing args() Function

```meadows
extern "meadows_args" args() -> i32;

func main() -> i32 {
    let count = args();
    print(count);  # Prints number of arguments
    return 0;
}
```

```bash
$ ./program.out foo bar
3  # Output: program name + 2 arguments = 3
```

## Security Tests

Security tests are in `scripts/test/run_security_tests.sh` and test:
- Command injection prevention
- Path traversal prevention
- File extension validation
- File size limits
- Error handling for unterminated strings
- Undefined variable/function detection

## Build with Tests

```bash
./build.sh tests          # Build with tests
cmake -B build -DBUILD_TESTS=ON && cd build && make -j4
```

## Code Coverage

```bash
./scripts/test/run_coverage.sh
```

Requires: `ENABLE_COVERAGE=ON` in CMake configuration.
