# Meadows Compiler Architecture

## Overview
Meadows is a simple compiled programming language that translates `.ms` source files into native executables via LLVM IR and clang++.

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
│ clang++ │ Native executable
└─────────┘
```

## Components

### Lexer (`src/lexer/`)
- **Files:** `Lexer.h`, `Lexer.cpp`, `Token.h`
- **Responsibility:** Tokenize source code into lexical tokens
- **Features:**
  - Keyword recognition (let, func, if, for, while, etc.)
  - Numeric and string literal parsing
  - Comment handling (# and // styles)
  - Line number tracking for error reporting

### Parser (`src/parser/`)
- **Files:** `Parser.h`, `Parser.cpp`
- **Responsibility:** Parse tokens into Abstract Syntax Tree
- **Technique:** Recursive descent parsing
- **Features:**
  - Operator precedence parsing (11 levels)
  - 11 statement types (let, func, if, for, while, return, etc.)
  - 11 expression types (literal, binary, unary, call, array, object, etc.)
  - Descriptive error messages with line numbers

### AST (`src/ast/`)
- **Files:** `AST.h`, `AST.cpp`
- **Node Types:**
  - Expressions: LiteralExpr, VarExpr, AssignExpr, BinaryExpr, UnaryExpr, LogicalExpr, CallExpr, IndexExpr, FieldAccessExpr, ArrayExpr, ObjectExpr
  - Statements: LetStmt, FuncStmt, IfStmt, ForStmt, WhileStmt, ReturnStmt, BreakStmt, ContinueStmt, BlockStmt, PrintStmt, ExprStmt
- **Pattern:** Visitor pattern for AST traversal

### CodeGen (`src/codegen/`)
- **Files:** `CodeGen.h`, `CodeGen.cpp`, `StringUtils.*`, `TypeUtils.*`, `MemoryUtils.*`
- **Responsibility:** Generate LLVM IR from AST
- **Features:**
  - Function definition generation
  - Variable allocation and scope management
  - Control flow (if/else, while, for loops)
  - Runtime validation (division by zero, array bounds)
  - String operations (concatenation, length)
  - Built-in functions: printf, malloc, strlen, strcpy, strcat

## Data Structures

### Variable Scope
- Stack-based scope management
- Linear search through scope stack (O(n) lookup)
- Allocations via `alloca` instruction

### String Handling
- String interning via `StringPool` singleton
- Compile-time constants for literal strings
- Runtime allocation for concatenated strings

### Memory Management
- RAII via `std::unique_ptr` for AST nodes
- LLVM resources managed via smart pointers
- malloc/free for runtime string concatenation

## Security Measures

### Input Validation
- File extension validation (.ms only)
- File size limit (10MB maximum)
- Path traversal prevention
- Dangerous character detection

### Secure Compilation
- fork() + execvp() instead of system()
- No shell command interpolation
- Validated output filenames

## Build System

### CMake Configuration
- C++17 standard
- LLVM 17.x dependency
- Optional test suite (Catch2)
- Optional code coverage

### Build Variants
- Debug: Symbols enabled, no optimization
- Release: Full optimization
- Tests: Catch2 test binary
