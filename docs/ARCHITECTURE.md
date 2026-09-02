# Meadows Compiler Architecture

## Overview

Meadows is a compiled programming language that translates `.ms` source files
into native executables via LLVM IR and clang++. The compiler is written in
C++17 and uses LLVM 17.x for code generation. A second, independent backend
(a tree-walking interpreter) powers the browser playground — see the
Interpreter section below for why.

## Compilation Pipeline (native backend)

```
Source Code (.ms)
     │
     ▼
┌──────────────────┐
│      Lexer        │ Tokenization → Token stream (+ import resolution)
└──────────────────┘
     │
     ▼
┌──────────────────┐
│      Parser       │ Recursive descent → AST
└──────────────────┘
     │
     ▼
┌──────────────────┐
│ Semantic Analyzer │ Name resolution, arity/type checks, warnings
└──────────────────┘
     │
     ▼
┌──────────────────┐
│      CodeGen       │ LLVM IR generation
└──────────────────┘
     │
     ▼
┌──────────────────┐
│      clang++       │ Native executable (.out)
└──────────────────┘
```

The browser playground replaces the last two stages with a single
tree-walking **Interpreter** (`src/interpreter/`) — see below.

## Module Structure

```
src/
  lexer/          - Tokenization (Lexer.h, Lexer.cpp, Token.h) and
                    CLI-only multi-file import resolution (ModuleResolver)
  parser/         - Syntax analysis (Parser.h, Parser.cpp)
  ast/            - AST node definitions (AST.h, AST.cpp)
  sema/           - Semantic analysis (SemanticAnalyzer.h, SemanticAnalyzer.cpp)
  codegen/        - LLVM IR generation (native backend)
  interpreter/    - Tree-walking interpreter (browser playground backend)
  wasm/           - Emscripten/embind bridge for the browser playground
  lsp/            - JSON diagnostics output for the --lsp-diagnostics CLI flag
  main/           - Entry point
  utils/          - Utilities (exceptions, diagnostics, warnings, timer, path validation)
```

## Components

### Lexer (`src/lexer/`)

**Files:** `Lexer.h`, `Lexer.cpp`, `Token.h`, `ModuleResolver.h`, `ModuleResolver.cpp`

**Responsibility:** Convert raw source code into a sequence of tokens.

**Features:**
- Keyword recognition: `let`, `func`, `if`, `for`, `while`, `return`, `break`, `continue`, `import`
- Numeric (including float) and string literal parsing
- Comment handling: `#` and `//` styles
- Line/column tracking for error reporting
- Custom exceptions: `LexicalException` with `SourceLocation`
- `ModuleResolver`: CLI-only, resolves `import "file.ms";` by textually
  splicing the imported file's tokens in place before parsing — see
  `docs/LANGUAGE.md`'s Imports section

**Token Types** (`src/lexer/Token.h`, `TokenType`):
- Keywords: `let`, `func`, `if`, `else`, `for`, `while`, `return`, `in`,
  `range`, `true`, `false`, `break`, `continue`, `import`. `print`, `len`,
  `str`, and `push` are **not** keywords — they're ordinary identifiers that
  the semantic analyzer and CodeGen recognize as built-in functions.
- Operators: `=, + - * / %, == != < > <= >=, && || !`
- Punctuation: `; : , . ( ) { } [ ]`
- Literals: integers, floats, strings, identifiers

### Parser (`src/parser/`)

**Files:** `Parser.h`, `Parser.cpp`

**Responsibility:** Parse token stream into Abstract Syntax Tree (AST).

**Technique:** Recursive descent parsing with operator precedence (11 levels).

**Statement Types (10):**
1. `LetStmt` - Variable declarations
2. `FuncStmt` - Function definitions
3. `IfStmt` - Conditional statements
4. `ForStmt` - For loops
5. `WhileStmt` - While loops
6. `ReturnStmt` - Return statements
7. `BlockStmt` - Block scoping
8. `ExprStmt` - Expression statements (includes `print(...)`, an ordinary call)
9. `BreakStmt` - Break from loops
10. `ContinueStmt` - Continue loops

**Expression Types (11):**
1. `LiteralExpr` - Int, Float, or Str literal (`LiteralKind`) — `true`/`false`
   parse directly to the Int literals `1`/`0`; there is no boolean or nil kind
2. `VarExpr` - Variable references
3. `AssignExpr` - Assignments (variable, array element, or object field target)
4. `BinaryExpr` - Binary operations
5. `UnaryExpr` - Unary operations
6. `LogicalExpr` - `&&` / `||`
7. `CallExpr` - Function calls
8. `IndexExpr` - Array/string indexing
9. `FieldAccessExpr` - Field access on objects
10. `ArrayExpr` - Array literals
11. `ObjectExpr` - Object literals

**Error Handling:**
- Custom `ParseException` with `SourceLocation`
- Panic mode recovery for error reporting
- MAX_CONSECUTIVE_ERRORS limit for graceful failure
- Descriptive error messages with line/column numbers

### AST (`src/ast/`)

**Files:** `AST.h`, `AST.cpp`

**Design Pattern:** Visitor pattern for AST traversal.

**Base Classes:**
- `Expr` - Base for all expressions
- `Stmt` - Base for all statements

**Visitor Interface:**
```cpp
class ExprVisitor {
  virtual void visitLiteralExpr(LiteralExpr& expr) = 0;
  virtual void visitVarExpr(VarExpr& expr) = 0;
  // ... other expressions
};

class StmtVisitor {
  virtual void visitLetStmt(LetStmt& stmt) = 0;
  virtual void visitFuncStmt(FuncStmt& stmt) = 0;
  // ... other statements
};
```

### Semantic Analyzer (`src/sema/`)

**Files:** `SemanticAnalyzer.h`, `SemanticAnalyzer.cpp`

**Responsibility:** Walk the AST (visitor pattern, like CodeGen) after
parsing and before code generation, catching errors that are structurally
valid but semantically wrong — the same class of errors both CodeGen and
the Interpreter would otherwise have to duplicate checks for.

**Checks:**
- Undefined variable / function references (`E3001`, `E3002`)
- Redefinition of a variable or function in the same scope (`E3003`, `E3004`)
- Built-in call argument counts (`E3006`)
- `break` / `continue` outside a loop, `return` outside a function
  (`E3009`, `E3010`, `E3011`)
- Unknown field access on an object literal (`E3012`)
- Warnings: unused variable, unreachable code, variable shadowing
  (`W6001`, `W6003`, `W6004`) — see `WarningManager` for severity control

### CodeGen (`src/codegen/`)

**Files:** `CodeGen.h`, `CodeGen.cpp`, `StringUtils.*`, `TypeUtils.*`, `SymbolTable.*`

**Responsibility:** Generate LLVM IR from AST.

**Features:**
- Function definition generation with proper signatures, including a
  signature-only pre-pass so forward/mutual recursion resolves
- Variable allocation using `alloca` instruction
- Control flow: if/else, while, for loops with proper branching
- Runtime validation: division by zero checks, array bounds checking
- String operations: concatenation, comparison, length, via libc functions
  declared at module init (`printf`, `malloc`, `free`, `strlen`, `strcpy`,
  `strcat`, `strcmp`, `snprintf`)
- Compile-time tracking of each array's length and each object's field
  shape, since arrays/objects carry no runtime type or length metadata (see
  the file-level doc comment in `CodeGen.h` for why)

**Code Generation Strategy:**
- RAII for LLVM resources (unique_ptr)
- `SymbolTable` (`src/codegen/SymbolTable.*`) for variable tracking across
  nested scopes
- Break/continue block handling for loops

### Interpreter (`src/interpreter/`)

**Files:** `Value.h`, `Value.cpp`, `Interpreter.h`, `Interpreter.cpp`

**Responsibility:** Execute the AST directly — a second, independent backend
from CodeGen. It exists because the native pipeline's final step (shelling
out to a real `clang++`) has no equivalent inside a browser's WebAssembly
sandbox; the browser playground (`src/wasm/WasmBridge.cpp`) uses this instead
of LLVM to actually run programs client-side.

**Value model:** a dynamically-typed `Value::Kind` — Int, Float, Str, Array,
Object, or Function (no separate boolean; matching CodeGen's i32-based
truthiness). Function values back interpreter-only first-class function
references (`let f = add;`), not supported by CodeGen — see "Functions as
values" in `docs/LANGUAGE.md`. Arrays and objects have reference semantics
via `shared_ptr`, matching the pointer aliasing the native backend gets from
`let b = a;`.

**Fidelity:** operator semantics (including `&&`/`||`, which forward operand
values rather than reducing to a strict boolean — see CodeGen's PHI nodes in
`visitLogicalExpr`) are matched against the generated IR, not reimplemented
from scratch, so interpreted output matches what the compiled binary prints.
Where the LLVM backend narrows the language for implementation reasons (see
"Current limitations" in `docs/LANGUAGE.md`), the interpreter does not carry
those restrictions — see the CHANGELOG for the specific superset behavior.

**Safety limits:** execution runs synchronously with no preemption available,
so a step counter, call-depth counter, and max string length bound a runaway
script (infinite loop / unbounded recursion) to a clean failure instead of a
hung or crashed page.

### Utilities (`src/utils/`)

**Key Components:**

| File | Purpose |
|------|---------|
| `Exceptions.h` | `MeadowsException` base + `SourceLocation`/`Diagnostic` structs + subclasses (see Exception Hierarchy below) |
| `ErrorCodes.h` | Error code enumeration (LEX_, PARSE_, SEM_, WARN_, SYS_) and `errorCodeToString()` |
| `DiagnosticsCollector.h` | Collect and manage diagnostics (100-diagnostic cap) |
| `WarningManager.h` | Warning configuration (OFF, DEFAULT, ALL, EXTRA) |
| `ErrorFormatter.h` | Rust-style formatted diagnostic output with source context |
| `PathValidation.h` | Shared file-path validation for the CLI entry file and `import` targets |
| `Timer.h` | Performance measurement utilities |

### LSP (`src/lsp/`)

**Files:** `LSPInterface.h`, `LSPInterface.cpp`

**Responsibility:** Emit diagnostics as LSP-shaped JSON on stdout when the
CLI is run with `--lsp-diagnostics <file>`. This is the entire integration
surface between the compiler and the real language server — see below.

This is deliberately thin: it converts `meadows::Diagnostic` objects (already
produced by the normal Lex → Parse → Sema pipeline) into JSON. It does not
implement the LSP protocol itself (no stdio JSON-RPC framing, no incremental
sync) — that's the job of the actual language server.

### Language Server & VS Code extension (`lsp/`)

Outside `src/` — a separate TypeScript project, not compiled into the
`Meadows` binary:

- **`lsp/meadows-lsp/`** — a real LSP server (`vscode-languageserver`) that
  spawns `Meadows --lsp-diagnostics <file>` per document change, parses the
  JSON it prints, and forwards it to the editor as LSP diagnostics
  (`src/compiler-bridge.ts`). Also provides hover and semantic-token
  providers.
- **`lsp/meadows-vscode/`** — a VS Code extension bundling the above server
  plus a TextMate grammar for syntax highlighting
  (`syntaxes/meadows.tmLanguage.json`).

See [`lsp/README.md`](../lsp/README.md) for building and running these.

## Data Structures

### Variable Scope

- Stack-based scope management
- Linear search through scope stack (O(n) lookup)
- Symbol table with nested scoping support
- Allocations via `alloca` instruction

### String Handling

- String interning via `StringPool` singleton
- Compile-time constants for literal strings
- Runtime allocation (malloc) for concatenated strings

### Memory Management

- AST nodes: `std::unique_ptr` (RAII)
- LLVM resources: smart pointers
- Runtime strings: malloc/free with cleanup

## Error Handling

### Exception Hierarchy

```
MeadowsException (base — code, message, SourceLocation, stack trace)
├── LexicalException  (lexer errors)
├── ParseException    (parser errors)
├── SemanticException (declared, currently unused — see note)
├── CodeGenException   (declared, currently unused — see note)
└── SystemException    (declared, currently unused — see note)
```

Only `LexicalException` and `ParseException` are actually thrown today.
The Semantic Analyzer reports through `DiagnosticsCollector` instead of
throwing (matching the Parser's non-fatal-error-collection design), and
CodeGen currently throws a plain `std::runtime_error` on fatal errors rather
than `CodeGenException`. `SystemException` has no throw sites at all — I/O
and path-validation failures (`PathValidation.h`) return a `bool` + error
string instead. The three unused subclasses are declared but not yet wired
up to their intended call sites.

### Error Codes

| Prefix | Category |
|--------|----------|
| LEX_ | Lexer errors |
| PARSE_ | Parser errors |
| SEM_ | Semantic errors |
| WARN_ | Warnings |
| SYS_ | System errors |

### Diagnostics

- Non-fatal error collection via `DiagnosticsCollector`
- Handler callbacks for custom processing
- Maximum diagnostic limit (100) to prevent runaway errors

## Security Measures

### Input Validation

Implemented in `src/utils/PathValidation.cpp`, shared by the CLI's entry
file and every `import` target:

| Check | Implementation |
|-------|---------------|
| File extension | `.ms` only |
| File size | 10 MB maximum (`kMaxSourceFileSize`) |
| Path traversal | Reject any path containing `..` |
| Dangerous chars | Reject `; \| & \` $ { } [ ] < > ! \ " ' ` and whitespace — `(` and `)` are allowed (e.g. `Program Files (x86)`), since they're harmless under exec-style invocation |

### Secure Compilation

- fork() + execvp() instead of system()
- No shell command interpolation
- Validated output filenames

## Build System

### CMake Configuration

```cmake
C++17 standard
LLVM 17.x dependency
Catch2 for testing (optional)
Code coverage (optional)
```

### Build Variants

| Variant | Purpose | Flags | Output |
|---------|---------|-------|--------|
| Debug | Development | `-DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON` | `build-debug/bin/Meadows` |
| Release | Production | `-DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON` | `build-release/bin/Meadows` |
| Tests | Testing | same as Debug | `build/tests/meadows_tests` |
| WASM | Browser playground | `-DBUILD_WASM=ON` (via `emcmake cmake`) | `bin/meadows.js` + `bin/meadows.wasm` |

The WASM variant is a separate CMake target (`meadows_wasm`) gated behind
`if(BUILD_WASM)` in `CMakeLists.txt` — it doesn't link LLVM at all, only the
LLVM-independent front end (Lexer, Parser, SemanticAnalyzer) plus the
Interpreter, bridged to JS via `src/wasm/WasmBridge.cpp`
(Emscripten/embind). See `.github/workflows/deploy-playground.yml` for the
exact build-and-deploy steps.

### Build Commands

```bash
./build.sh debug     # Debug build
./build.sh release   # Release build
./build.sh tests     # Build with tests
./test.sh           # Run all tests
```

## Testing

### Test Structure

```
tests/
├── unit/            # Unit tests by module (Catch2)
│   ├── ast/
│   ├── codegen/
│   ├── interpreter/
│   ├── lexer/
│   ├── parser/
│   ├── sema/
│   └── utils/
├── integration/      # Full .ms programs + expected output
├── edge_cases/        # Boundary/pathological input programs
├── performance/        # Performance regression checks
└── security/           # Path/injection/fuzz tests
```

### Test Framework

- **Catch2 v3.x** for unit tests
- Tags: `[lexer]`, `[parser]`, `[exceptions]`, `[diagnostics]`, `[warnings]`
- Integration tests via `.ms` files

### Security Tests

- Command injection prevention
- Path traversal prevention
- File extension validation
- File size limits
- Error handling verification
