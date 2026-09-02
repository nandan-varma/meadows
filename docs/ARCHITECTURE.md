# Meadows Compiler Architecture

## Overview

Meadows is a compiled programming language that translates `.ms` source files into native executables via LLVM IR and clang++. The compiler is written in C++17 and uses LLVM 17.x for code generation.

## Compilation Pipeline

```
Source Code (.ms)
     │
     ▼
┌─────────┐
│  Lexer  │ Tokenization → Token stream
└─────────┘
     │
     ▼
┌─────────┐
│ Parser  │ Recursive descent → AST
└─────────┘
     │
     ▼
┌──────────┐
│ CodeGen  │ LLVM IR generation
└──────────┘
     │
     ▼
┌─────────┐
│ clang++ │ Native executable (.out)
└─────────┘
```

## Module Structure

```
src/
  lexer/          - Tokenization (Lexer.h, Lexer.cpp, Token.h)
  parser/         - Syntax analysis (Parser.h, Parser.cpp)
  ast/            - AST node definitions (AST.h, AST.cpp)
  codegen/        - LLVM IR generation
  interpreter/    - Tree-walking interpreter (browser playground backend)
  wasm/           - Emscripten/embind bridge for the browser playground
  lsp/            - Language Server Protocol support
  main/           - Entry point
  utils/          - Utilities (exceptions, diagnostics, warnings, timer)
```

## Components

### Lexer (`src/lexer/`)

**Files:** `Lexer.h`, `Lexer.cpp`, `Token.h`

**Responsibility:** Convert raw source code into a sequence of tokens.

**Features:**
- Keyword recognition: `let`, `func`, `if`, `for`, `while`, `return`, `break`, `continue`
- Numeric and string literal parsing
- Comment handling: `#` and `//` styles
- Line/column tracking for error reporting
- Custom exceptions: `LexicalException` with `SourceLocation`

**Token Types:**
- Keywords (let, func, if, for, while, return, break, continue, print, true, false, nil)
- Operators (=, +, -, *, /, ==, !=, <, >, <=, >=, &&, ||)
- Punctuation (;, :, ,, (, ), {, }, [, ])
- Literals (integers, strings, identifiers)

### Parser (`src/parser/`)

**Files:** `Parser.h`, `Parser.cpp`

**Responsibility:** Parse token stream into Abstract Syntax Tree (AST).

**Technique:** Recursive descent parsing with operator precedence (11 levels).

**Statement Types (11):**
1. `LetStmt` - Variable declarations
2. `FuncStmt` - Function definitions
3. `IfStmt` - Conditional statements
4. `ForStmt` - For loops
5. `WhileStmt` - While loops
6. `ReturnStmt` - Return statements
7. `BlockStmt` - Block scoping
8. `PrintStmt` - Print statements
9. `ExprStmt` - Expression statements
10. `BreakStmt` - Break from loops
11. `ContinueStmt` - Continue loops

**Expression Types (11):**
1. `LiteralExpr` - Literals (numbers, strings, booleans, nil)
2. `VarExpr` - Variable references
3. `AssignExpr` - Assignments (variable, array element, or object field target)
4. `BinaryExpr` - Binary operations
5. `UnaryExpr` - Unary operations
6. `LogicalExpr` - Logical operations (and, or)
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

### CodeGen (`src/codegen/`)

**Files:** `CodeGen.h`, `CodeGen.cpp`, `StringUtils.*`, `TypeUtils.*`, `MemoryUtils.*`

**Responsibility:** Generate LLVM IR from AST.

**Features:**
- Function definition generation with proper signatures
- Variable allocation using `alloca` instruction
- Control flow: if/else, while, for loops with proper branching
- Runtime validation: division by zero checks, array bounds checking
- String operations: concatenation, length via library functions
- Built-in functions: printf, malloc, free, strlen, strcpy, strcat

**Code Generation Strategy:**
- RAII for LLVM resources (unique_ptr)
- Symbol table for variable tracking
- Scope stack for nested scoping
- Break/continue block handling for loops

### Interpreter (`src/interpreter/`)

**Files:** `Value.h`, `Value.cpp`, `Interpreter.h`, `Interpreter.cpp`

**Responsibility:** Execute the AST directly — a second, independent backend
from CodeGen. It exists because the native pipeline's final step (shelling
out to a real `clang++`) has no equivalent inside a browser's WebAssembly
sandbox; the browser playground (`src/wasm/WasmBridge.cpp`) uses this instead
of LLVM to actually run programs client-side.

**Value model:** a dynamically-typed `Value` (Int/Str/Array/Object, no
separate boolean — matching CodeGen's i32-based truthiness). Arrays and
objects have reference semantics via `shared_ptr`, matching the pointer
aliasing the native backend gets from `let b = a;`.

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
| `Exceptions.h` | Custom exceptions: `LexicalException`, `ParseException`, `MeadowsException` |
| `ErrorCodes.h` | Error code enumeration (LEX_, PARSE_, SEM_, WARN_, SYS_) |
| `DiagnosticsCollector.h` | Collect and manage diagnostics |
| `WarningManager.h` | Warning configuration (OFF, DEFAULT, ALL, EXTRA) |
| `Timer.h` | Performance measurement utilities |
| `SourceLocation.h` | File location tracking for errors |

### LSP (`src/lsp/`)

**Files:** `LSPInterface.h`, `LSPInterface.cpp`

**Features:**
- Language Server Protocol integration
- Compiler interface for IDE features
- Diagnostics reporting

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
MeadowsException (base)
├── LexicalException (lexer errors)
├── ParseException (parser errors)
└── MeadowsException (general errors)
```

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

| Check | Implementation |
|-------|---------------|
| File extension | `.ms` only |
| File size | 10MB maximum |
| Path traversal | Reject `../` paths |
| Dangerous chars | Reject `;&`$\(){}[]<>!` |

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

| Variant | Purpose | Flags |
|---------|---------|-------|
| Debug | Development | Symbols, no optimization |
| Release | Production | Full optimization |
| Tests | Testing | Catch2 enabled |

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
├── unit/           # Unit tests by module
│   ├── lexer/      # Lexer tests
│   ├── parser/     # Parser tests
│   ├── codegen/    # CodeGen tests
│   └── utils/      # Utility tests
├── integration/    # Full program tests
└── security/       # Security tests
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
