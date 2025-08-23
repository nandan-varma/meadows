# Meadows Compiler

A modular compiler frontend for a Python-like programming language, built with C++ and LLVM.

## Features

### Lexical Analysis
- **Tokens**: Identifiers, numbers (integers/floats), strings, operators, keywords
- **Indentation-based blocks**: INDENT/DEDENT tokens for Python-style block structure
- **Comments**: Line comments with `#`
- **Operators**: Arithmetic (`+`, `-`, `*`, `/`, `%`, `**`), comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`), logical (`and`, `or`, `not`)
- **Keywords**: `def`, `class`, `if`, `else`, `while`, `for`, `in`, `return`, `break`, `continue`, `pass`, etc.

### Syntax Analysis
- **Expressions**: Binary operations, unary operations, function calls, attribute access, indexing
- **Statements**: Assignments, if/else, while loops, for loops, function definitions, class definitions
- **Error Recovery**: Synchronization on statement boundaries for better error reporting
- **Precedence Climbing**: Proper operator precedence and associativity

### Abstract Syntax Tree (AST)
- **Modular Design**: Separate Expression and Statement hierarchies
- **Visitor Pattern**: Extensible traversal system for AST processing
- **Location Tracking**: Source location information for error reporting

### Error Reporting
- **Contextual Errors**: Line/column information with source context
- **Error Levels**: Warning, Error, and Fatal error categories
- **Graceful Degradation**: Continue parsing after recoverable errors

## Architecture

```
src/
├── lexer/          # Tokenization
│   ├── Token.h/cpp
│   └── Lexer.h/cpp
├── ast/            # Abstract Syntax Tree
│   ├── Expression.h
│   ├── Statement.h
│   └── ASTImpl.cpp
├── parser/         # Syntax Analysis
│   └── Parser.h/cpp
├── visitor/        # AST Traversal
│   ├── ASTVisitor.h
│   └── ASTPrinter.h/cpp
├── common/         # Utilities
│   └── ErrorReporter.h/cpp
├── Compiler.h/cpp  # Main Driver
└── main.cpp        # Entry Point
```

## Supported Language Features

### Data Types
- Integers: `42`, `-123`
- Floats: `3.14`, `-2.5`
- Strings: `"hello"`, `'world'`
- Booleans: `True`, `False`
- None: `None`

### Control Flow
```python
# Conditional statements
if x > 0:
    print("positive")
else:
    print("non-positive")

# While loops
while i < 10:
    print(i)
    i = i + 1

# For loops
for item in collection:
    process(item)
```

### Functions
```python
def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n - 1) + fibonacci(n - 2)

def greet(name, greeting="Hello"):
    return greeting + ", " + name
```

### Classes
```python
class Calculator:
    def __init__(self, initial_value=0):
        self.value = initial_value
    
    def add(self, x):
        self.value = self.value + x
        return self.value
```

### Expressions
```python
# Arithmetic with proper precedence
result = (a + b) * c ** 2 - func(x, y)

# Method chaining
value = calculator.add(10).multiply(2).get_result()

# Attribute access
user.profile.name = "John"
```

## Building

### Prerequisites
- CMake 3.15+
- C++17 compatible compiler
- LLVM development libraries

### Build Instructions
```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Run
./build/meadows [source_file.py]
```

## Usage

### Compile a file
```bash
./build/meadows my_program.py
```

### Interactive demo
```bash
./build/meadows
```

This will run built-in test cases and demonstrate the compiler's capabilities.

## Example Output

```
=== AST ===
Program:
  FunctionDefinition: factorial
    Parameters:
      n
    Body:
      Block:
        IfStatement:
          Condition:
            BinaryExpression(Identifier(n) <= IntegerLiteral(1))
          Then:
            Block:
              ReturnStatement:
                IntegerLiteral(1)
          Else:
            Block:
              ReturnStatement:
                BinaryExpression(Identifier(n) * FunctionCall(Identifier(factorial), [BinaryExpression(Identifier(n) - IntegerLiteral(1))]))
```

## Extensibility

The modular design makes it easy to extend:

### Adding New AST Node Types
1. Add new node class inheriting from `Expression` or `Statement`
2. Add visitor method to `ASTVisitor`
3. Implement `accept()` method
4. Update parser to create the new nodes

### Adding New Visitors
1. Inherit from `ASTVisitor`
2. Implement all visitor methods
3. Use for code generation, optimization, analysis, etc.

### Adding New Token Types
1. Add to `TokenType` enum
2. Update lexer recognition logic
3. Update parser if needed

## Future Extensions

- **Semantic Analysis**: Type checking, symbol tables, scope analysis
- **Code Generation**: LLVM IR generation for executable code
- **Optimizations**: Constant folding, dead code elimination
- **Advanced Features**: List comprehensions, decorators, modules
- **Standard Library**: Built-in functions and data structures

## License

This project is for educational purposes demonstrating compiler construction techniques.