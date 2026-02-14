# AGENTS.md - Coding Guidelines for Meadows Compiler

Guidelines for AI agents working on the Meadows compiler codebase.

## Build Commands

```bash
# Build targets
./build.sh                  # Build everything (debug + release + tests)
./build.sh debug            # Debug build only
./build.sh release          # Release build only
./build.sh tests            # Test executables only
./build.sh clean            # Clean all build artifacts

# CMake directly
cmake -B build -DBUILD_TESTS=ON && cmake --build build -j$(nproc)
```

## Test Commands

```bash
# Test suites
./test.sh                   # All tests
./test.sh unit              # Unit tests only
./test.sh integration       # Integration tests (.ms files)
./test.sh security          # Security tests
./test.sh coverage          # Coverage report

# Run specific test by name
./build/tests/meadows_tests "WarningManager setLevel OFF"

# Run tests by tag
./build/tests/meadows_tests "[lexer]"
./build/tests/meadows_tests "[parser]"
./build/tests/meadows_tests "[codegen]"

# Catch2 options
./build/tests/meadows_tests -l       # List all tests
./build/tests/meadows_tests -s       # Show stdout
./build/tests/meadows_tests -r compact  # Compact output
```

## Code Quality

```bash
./scripts/dev/format.sh [--check]       # Format code (clang-format)
./scripts/dev/run_static_analysis.sh    # Static analysis
```

## Code Style

### Formatting (from .clang-format)
- 2-space indentation
- 80 character line limit
- Braces on same line: `if (x) {`
- Pointer alignment: right (`int* ptr`)
- One declaration per line
- C++17 standard

### Import Order
```cpp
// 1. Standard library
#include <string>
#include <vector>

// 2. LLVM
#include <llvm/IR/Type.h>

// 3. Project
#include "../ast/AST.h"
```

### Naming Conventions
| Type | Convention | Example |
|------|-----------|---------|
| Classes/Structs | PascalCase | `Lexer`, `Parser` |
| Functions | camelCase | `tokenize()`, `parseExpr()` |
| Variables | camelCase | `currentToken` |
| Constants | UPPER_SNAKE_CASE | `MAX_FILE_SIZE` |
| Private members | trailing underscore | `context_`, `module_` |
| Files | PascalCase | `Lexer.h`, `Parser.cpp` |

### Types and Memory
- Use `std::unique_ptr` for AST nodes
- Use `std::string` (no raw char*)
- Use `std::vector` for collections
- Use `std::unordered_map` for hash lookups
- Never use raw `new`/`delete` - use smart pointers
- Use `auto` for iterators and lambdas
- Use `constexpr` for compile-time constants

### Error Handling
- Use exceptions from `src/utils/Exceptions.h`:
  - `LexicalException` - lexer errors
  - `ParseException` - parser errors
  - `MeadowsException` - general errors
- Include line numbers: `"Error at line " + std::to_string(line)`
- Use `DiagnosticsCollector` for non-fatal errors
- Exit codes: 1 = usage error, 2 = critical error

### Class Design
- Virtual destructor for base classes: `virtual ~Expr() = default;`
- Use visitor pattern for AST traversal
- All AST nodes inherit from `Expr` or `Stmt`
- Use `override` for all overridden methods
- Keep visitor methods const where possible

### Security
- Never use `system()`; use `fork()` + `execvp()`
- Validate paths for traversal (`..`, dangerous chars)
- Check file sizes (10MB max via `MAX_ALLOC_SIZE`)
- Reject dangerous chars: `;|&`$
(){}[]<>!
`
- Use `MemoryUtils.h` for safe allocation

## Project Structure

```
src/
  lexer/          - Tokenization
  parser/         - Syntax analysis
  ast/            - AST node definitions
  codegen/        - LLVM IR generation
  lsp/            - Language Server Protocol
  utils/          - Exceptions, diagnostics, warnings
  main/           - Entry point
  stdlib/         - Standard library (c/ and std/)
  types/          - Type system
  modules/        - Module resolution

tests/
  unit/           - Unit tests (Catch2)
  integration/    - Full program tests (.ms)
  security/       - Security and fuzz tests
```

## Common Patterns

### Adding AST Nodes
1. Define class in `src/ast/AST.h` inheriting from `Expr` or `Stmt`
2. Add visitor methods to `ExprVisitor`/`StmtVisitor`
3. Implement `accept()` in `src/ast/AST.cpp`
4. Add visitor implementation in `src/codegen/CodeGen.cpp`
5. Add parser support in `src/parser/Parser.cpp`
6. Add lexer token in `src/lexer/Token.h` if needed
7. Add unit tests in `tests/unit/<module>/`

### Testing
- Use Catch2 v3.x
- One test file per module: `Lexer.test.cpp`
- Use tags: `[lexer]`, `[parser]`, `[codegen]`
- Include positive and negative test cases
- Test edge cases: empty input, large files, nesting

## Git Workflow
- Commit messages: imperative mood, 50-char summary
- One logical change per commit
- Run tests before committing: `./test.sh unit`
- Don't commit build artifacts
- Include test coverage for new features

See README.md for complete roadmap.

## Performance Tips
- Profile before optimizing
- Use `std::unordered_map` for O(1) lookups
- Reserve vector capacity when size known
- Avoid O(n²) string concatenation
