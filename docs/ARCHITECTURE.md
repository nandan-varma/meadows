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
3. `AssignExpr` - Variable assignments
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
## Standard Library

### C Standard Library (`src/stdlib/c/`)

**Files:** `meadows_stdlib.h`, `meadows_stdlib.c`

**Purpose:** C implementation of runtime functions used by Meadows programs.

**Key Functions:**

| Function | Description |
|----------|-------------|
| `meadows_args()` | Returns the argument count |
| `meadows_getarg(n)` | Returns the nth argument as string |
| `meadows_set_args(argc, argv)` | Stores argc/argv globally |
| `meadows_opendir(path)` | Opens directory, returns handle (i64) |
| `meadows_readdir(dir_ptr)` | Reads next entry, returns name |
| `meadows_closedir(dir_ptr)` | Closes directory |
| `meadows_mkdir(path, mode)` | Creates directory |
| `meadows_rmdir(path)` | Removes directory |
| `meadows_is_directory(path)` | Checks if path is directory |

### Meadows Standard Library (`src/stdlib/std/`)

**Modules:** `io.ms`, `string.ms`, `dir.ms`, `math.ms`, `time.ms`, `array.ms`, `os.ms`

**Module Pattern:**
```meadows
import std.io;
import std.dir;

let file = fopen("test.txt", "r");
let content = read_file("test.txt");
```

**Directory Operations (`std/dir.ms`):**
```meadows
import std.dir;

let dir = opendir(".");
let entry = readdir(dir);
closedir(dir);
```

**Command-Line Arguments (`extern` declarations):**
```meadows
extern "meadows_args" args() -> i32;
extern "meadows_getarg" getarg(n: i32) -> string;

func main() -> i32 {
    let count = args();           # Number of arguments
    let arg0 = getarg(0);        # Program name
    let arg1 = getarg(1);        # First argument
    print(count);
    return 0;
}
```

### CLI Commands (`examples/cli/commands/`)

**Implemented Commands:**

| Command | Description |
|---------|-------------|
| `ls.ms` | List directory contents |
| `cat.ms` | Concatenate and print files |
| `echo.ms` | Print arguments |
| `head.ms` | Print first N lines |
| `tail.ms` | Print last N lines |
| `wc.ms` | Word, line, and character count |

**Building Commands:**
```bash
cd examples/cli/commands
/Users/nandan/dev/meadows/build/bin/Meadows ls.ms
./ls.ms.out .
```

## Entry Point Architecture

### Main Wrapper Pattern

On macOS, the system's C runtime calls `_main` as the entry point. The Meadows compiler generates:

1. **Wrapper Function (`main` → `_main`):**
   - Receives `argc` and `argv` from the runtime
   - Calls `meadows_set_args()` to store globally
   - Calls user's `main()` function (renamed to `_meadows_user_main`)
   - Returns exit code

2. **User's Main Function (`_meadows_user_main`):**
   - Renamed during code generation to avoid symbol conflicts
   - Can use `args()` and `getarg()` to access command-line arguments
   - Standard function body as defined in source

**Generated IR Pattern:**
```llvm
define i32 @main(i32 %argc, ptr %argv) {
entry:
  call void @meadows_set_args(i32 %argc, ptr %argv)
  call i32 @_meadows_user_main()
  ret i32 0
}

define i32 @_meadows_user_main() {
  ; User's code here
  ; Can call meadows_args() to get argc
}

define i32 @meadows_args() {
  ; Returns global_argc
}
```