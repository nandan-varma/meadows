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
- `MeadowsException` (base, in `src/utils/Exceptions.h`) carries an
  `ErrorCode`, message, and `SourceLocation`. Subclasses: `LexicalException`,
  `ParseException` are actually thrown today; `SemanticException`,
  `CodeGenException`, `SystemException` are declared but not yet wired to
  their intended call sites (SemanticAnalyzer reports via
  `DiagnosticsCollector` instead; CodeGen throws plain `std::runtime_error`)
  — see `docs/ARCHITECTURE.md`'s Exception Hierarchy for the full picture.
- Prefer `DiagnosticsCollector::reportError()`/`reportWarning()` over
  throwing when the caller can keep parsing/analyzing after the error —
  that's what lets the CLI report multiple problems in one pass.
- Exit codes: 0 = success, 1 = any failure. There's no separate code for
  different failure categories.

### Class Design
- Virtual destructor for base classes: `virtual ~Expr() = default;`
- Use visitor pattern for AST traversal
- All AST nodes inherit from `Expr` or `Stmt`
- Use `override` for all overridden methods
- Keep visitor methods const where possible

### Security
- Never use `system()`; use `fork()` + `execvp()`
- Validate paths (CLI entry file and `import` targets) via
  `src/utils/PathValidation.h` — don't duplicate its checks
- File size cap: 10MB (`kMaxSourceFileSize`)
- Dangerous-char rejection excludes `(` `)` deliberately (real paths like
  `Program Files (x86)`) — see `PathValidation::hasDangerousChars` for the
  exact set before changing it

## Testing Guidelines

- Place tests in `tests/unit/<module>/`, one file per module (e.g.
  `Lexer.test.cpp`), and register it in `tests/CMakeLists.txt`
- Include both positive and negative cases; test boundaries (empty input,
  large input, max nesting)
- See [docs/TESTING.md](docs/TESTING.md) for tags, running a single test,
  and the full test-file template

## Module Structure

```
src/
  lexer/          - Tokenization + CLI-only import resolution (ModuleResolver)
  parser/         - Syntax analysis
  ast/            - AST node definitions
  sema/           - Semantic analysis (name resolution, arity/type checks)
  codegen/        - LLVM IR generation (native backend)
  interpreter/    - Tree-walking interpreter (browser playground backend)
  wasm/           - Emscripten/embind bridge for the browser playground
  lsp/            - JSON diagnostics for the --lsp-diagnostics CLI flag
  utils/          - Exceptions, diagnostics, warnings, timer, path validation
  main/           - Entry point

tests/
  unit/           - Unit tests by module (ast, codegen, interpreter, lexer,
                    parser, sema, utils)
  integration/    - Full program tests (.ms files)
  security/       - Security and fuzz tests
  edge_cases/      - Boundary/pathological input programs
  performance/     - Performance regression benchmarks
```

Note: `lsp/` at the **repo root** (not under `src/`) is a separate
TypeScript project — the real language server and VS Code extension. See
[`lsp/README.md`](lsp/README.md).

## Common Patterns

### Adding AST Nodes
1. Define class in `src/ast/AST.h` inheriting from `Expr` or `Stmt`
2. Add visitor methods to `ExprVisitor`/`StmtVisitor`
3. Implement `accept()` in `src/ast/AST.cpp`
4. Add parser support in `src/parser/Parser.cpp`/`ParserExpressions.cpp`
5. Add lexer token in `src/lexer/Token.h` if needed
6. Add a visitor implementation everywhere the interface requires one —
   `SemanticAnalyzer` (`src/sema/`), `CodeGen` (`src/codegen/`),
   `Interpreter` (`src/interpreter/`), and `ASTPrinter` (`src/utils/`).
   CodeGen and the Interpreter are two independent backends for the same
   language — if a feature lands in one but not the other, it silently works
   in the CLI but not the browser playground, or vice versa. Keep their
   observable behavior identical for anything both support; where the native
   backend has to narrow (e.g. no closures, i32-only arrays), reject it at
   compile time with a clear error rather than miscompiling.

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
- Commit messages: imperative mood, 50-char summary (e.g. "Add lexer token
  validation", not "Added lexer token validation")
- One logical change per commit
- Run `./test.sh unit` before committing; run `./test.sh security` too for
  anything touching path validation, subprocess invocation, or parsing of
  untrusted input
- Don't commit build artifacts or `.vsix` files
- New features need unit test coverage; new language features also need an
  integration test under `tests/integration/`

## Performance Tips
- Profile before optimizing
- Use `std::unordered_map` for O(1) lookups
- Reserve vector capacity when size known
- Avoid O(n²) string concatenation
