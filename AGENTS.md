# AGENTS.md - Guidelines for Coding Agents

Guidelines for AI agents working on the Meadows compiler codebase.

## Quick Reference Commands

### Building
```bash
./build.sh tests          # Full build with tests
./build.sh debug/release  # Specific build type
cmake -B build -DBUILD_TESTS=ON && cmake --build build -j$(nproc)
```

### Testing
```bash
./test.sh                     # All tests (unit + integration + security)
./test.sh unit                # Unit tests only
./test.sh integration         # Integration tests only
./test.sh security            # Security tests only

# Single test case (exact name)
./build/tests/meadows_tests "WarningManager setLevel OFF"

# By tag
./build/tests/meadows_tests "[lexer]"
./build/tests/meadows_tests "[parser]"
./build/tests/meadows_tests "[diagnostics]"
./build/tests/meadows_tests "[warnings]"

# Catch2 options
./build/tests/meadows_tests -l                 # List all tests
./build/tests/meadows_tests -s                 # Show stdout
./build/tests/meadows_tests -r compact         # Compact output
```

### Code Quality
```bash
./scripts/dev/format.sh [--check]       # Format code
./scripts/dev/run_static_analysis.sh    # Static analysis
cmake -B build -DENABLE_COVERAGE=ON && ./build.sh tests  # Coverage
```

## Code Style Guidelines

### Formatting (from .clang-format)
- 2-space indentation
- 80 character line limit
- Braces on same line: `if (x) {`
- Pointer alignment: right (`int* ptr`)
- One declaration per line

### Imports
```cpp
// Order: standard library → LLVM → project
#include <string>
#include <vector>
#include <llvm/IR/Type.h>
#include "../ast/AST.h"
```

### Naming Conventions
| Type | Convention | Example |
|------|-----------|---------|
| Classes | PascalCase | `Lexer`, `Parser` |
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
- Custom exceptions in `src/utils/Exceptions.h`:
  - `LexicalException` - lexer errors with `SourceLocation`
  - `ParseException` - parser errors with `SourceLocation`
  - `MeadowsException` - general errors with `ErrorCode`
- Include line numbers: `"Error at line " + std::to_string(line)`
- Use `DiagnosticsCollector` for non-fatal error collection
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
- Reject dangerous chars: `;|&`$\(){}[]<>!\`
- Use `MemoryUtils.h` for safe allocation

## Testing Guidelines

- Use Catch2 v3.x for unit tests
- Place tests in `tests/unit/<module>/`
- One test file per module: `Lexer.test.cpp`
- Use tags: `[lexer]`, `[parser]`, `[exceptions]`, `[diagnostics]`
- Include positive and negative test cases
- Test edge cases: empty input, large files, nesting

## Module Structure

```
src/
  lexer/          - Tokenization
  parser/         - Syntax analysis
  ast/            - AST node definitions
  codegen/        - LLVM IR generation
  lsp/            - Language Server Protocol support
  utils/          - Utilities (exceptions, diagnostics, warnings, timer)
  main/           - Entry point

tests/
  unit/           - Unit tests by module
  integration/    - Full program tests (.ms files)
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

### Adding Compiler Pass
1. Add method declaration to appropriate class
2. Implement in corresponding `.cpp` file
3. Add unit tests in `tests/unit/<module>/`
4. Update `CMakeLists.txt` if new source files

### Adding Exceptions
1. Define error code in `src/utils/ErrorCodes.h`
2. Add exception class in `src/utils/Exceptions.h` if needed
3. Use `DiagnosticsCollector::reportError()` for non-fatal errors
4. Add tests in `tests/unit/utils/Exceptions.test.cpp`

## Git Workflow
- Commit messages: imperative mood, 50-char summary
- One logical change per commit
- Run tests before committing: `./test.sh unit`
- Don't commit build artifacts or `.vsix` files
- Include test coverage for new features

## Performance Tips
- Profile before optimizing
- Use `std::unordered_map` for O(1) lookups
- Reserve vector capacity when size known
- Avoid O(n²) string concatenation
