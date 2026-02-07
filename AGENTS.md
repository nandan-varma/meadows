# AGENTS.md - Guidelines for Coding Agents

This document provides guidelines for AI agents working on the Meadows compiler codebase.

## Quick Reference Commands

### Building
```bash
# Full build with tests
./build.sh tests

# Clean rebuild
rm -rf build && ./build.sh tests

# Debug build
./build.sh

# Release build
cmake -B build-release -DCMAKE_BUILD_TYPE=Release && cmake --build build-release
```

### Testing
```bash
# All tests
./test.sh

# Unit tests only
./test.sh unit

# Integration tests only
./test.sh integration

# Security tests only
./test.sh security

# Single test case (using test binary directly)
./build/tests/meadows_tests "[lexer]"           # Run lexer tests
./build/tests/meadows_tests "Lexer handles strings"  # Specific test
./build/tests/meadows_tests "[parser]"           # Run parser tests

# Run with Catch2 options
./build/tests/meadows_tests --list-tests        # List all tests
./build/tests/meadows_tests --reporter compact  # Compact output
./build/tests/meadows_tests -d yes             # Show durations
```

### Performance Benchmarks
```bash
./scripts/test/run_benchmarks.sh
```

### Static Analysis
```bash
./scripts/dev/run_static_analysis.sh
```

### Code Formatting
```bash
./format.sh  # If available
clang-format -i src/**/*.cpp src/**/*.h  # Manual formatting
```

## Code Style Guidelines

### Imports and Dependencies
- Use standard C++17 libraries where possible
- LLVM headers included via angle brackets: `#include <llvm/IR/...>`
- Project headers use relative paths: `#include "../ast/AST.h"`
- No external dependencies beyond LLVM and Catch2 (for tests)

### Formatting
- Follow `.clang-format` configuration if present
- 2-space indentation for new code (matches existing style in meadows.cpp)
- Maximum line length: 100 characters
- Braces on same line for control structures: `if (...) {`

### Types
- Use `std::unique_ptr` for automatic memory management
- Use `std::string` for string handling (no raw char*)
- Use `std::vector` for collections
- Use `std::map` for key-value mappings
- Prefer `auto` for iterator types and lambda returns
- Use `constexpr` for compile-time constants

### Naming Conventions
- **Classes**: PascalCase (`Lexer`, `Parser`, `CodeGen`)
- **Functions**: camelCase (`tokenize()`, `parseExpr()`)
- **Variables**: camelCase (`currentToken`, `sourceFile`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_FILE_SIZE`)
- **Private Members**: trailing underscore allowed (`context_`, `module_`)
- **Files**: PascalCase for class files (`Lexer.h`, `Parser.cpp`)

### Error Handling
- Use `std::runtime_error` for recoverable errors
- Include line numbers in error messages
- Always validate input parameters
- Use exceptions for parse/compile errors
- Clean up resources via RAII (no manual `delete`)

### Class Design
- Base classes define virtual destructor: `virtual ~Expr() = default;`
- Use visitor pattern for AST traversal
- All AST nodes inherit from `Expr` or `Stmt`
- Visitor classes implement both `ExprVisitor` and `StmtVisitor`

### Security
- Never use `system()` or shell commands; use `fork()` + `execvp()`
- Validate all file paths for path traversal (`..`, dangerous chars)
- Check file sizes before processing (10MB max)
- Reject dangerous characters in filenames: `;|&`$\(){}[]<>!\`

### Testing
- Use Catch2 v3.x for unit tests
- Follow existing test patterns in `tests/unit/`
- One test file per module (`Lexer.test.cpp`, `Parser.test.cpp`)
- Include both positive and negative test cases
- Test edge cases: empty input, large files, deeply nested structures

### Module Structure
```
src/
  lexer/      - Tokenization
  parser/     - Syntax analysis
  ast/        - AST node definitions
  codegen/    - LLVM IR generation
  main/       - Entry point
tests/
  unit/       - Unit tests
  integration/- Integration tests
  security/   - Security tests
scripts/
  dev/        - Development scripts
  test/       - Test scripts
```

### Git Workflow
- Commit messages: imperative mood, 50-char summary
- One logical change per commit
- Run tests before committing: `./test.sh`
- Don't commit build artifacts or temporary files
