# Meadows Compiler

A modular compiler frontend for a Python-like programming language, built with C++ and LLVM. Features both native compilation and WebAssembly support for browser-based development.

## 🌐 Online IDE

Try Meadows in your browser! Visit our online IDE at:
**[https://nandan-varma.github.io/meadows](https://nandan-varma.github.io/meadows)**

Features:
- Real-time compilation to LLVM IR
- Interactive code editor with syntax highlighting
- AST and token visualization
- Pre-loaded examples
- No installation required

## Usage

The Meadows compiler supports various command-line options for different compilation modes and outputs.

### Basic Syntax
```bash
./build/meadows [options] [input_file]
```

### Command-line Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-i <file>` | `--input <file>` | Specify input source file |
| `-o <file>` | `--output <file>` | Specify output executable file (enables compile mode) |
| `-c` | `--compile` | Compile to executable |
| `-p` | `--parse-only` | Parse and show AST only |
| `-t` | `--tokens` | Show tokenization output |
| `-h` | `--help` | Show help message |

### Usage Examples

#### Interactive Demo
```bash
./build/meadows
```
Runs the built-in demo with test cases and demonstrates the compiler's capabilities.

#### Parse and Show AST
```bash
./build/meadows my_program.py
./build/meadows --input my_program.py --parse-only
```
Parses the source file and displays the Abstract Syntax Tree.

#### Compile to Executable
```bash
./build/meadows -i my_program.py -o my_program
./build/meadows my_program.py --output my_program --compile
```
Compiles the source file to an executable binary.

#### Show Tokenization
```bash
./build/meadows --input my_program.py --tokens
```
Shows the tokenization output along with the AST.

#### Combined Options
```bash
./build/meadows my_program.py --tokens --parse-only
```
Shows both tokenization and AST output without compiling.

## Building

### Prerequisites
- CMake 3.15+
- C++17 compatible compiler
- LLVM development libraries

### Native Build
```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Run
./build/meadows [source_file.py]
```

### WebAssembly Build

For browser-based compilation, build the WebAssembly version:

#### Prerequisites
- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)

#### Quick Start
```bash
# Install Emscripten (one-time setup)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build WebAssembly version
./build_wasm.sh build

# Start development server
./build_wasm.sh serve
# Open http://localhost:8000 in your browser
```

#### Build Commands
```bash
./build_wasm.sh build    # Build WebAssembly module
./build_wasm.sh clean    # Clean previous builds  
./build_wasm.sh test     # Build and test module
./build_wasm.sh serve    # Build and start development server
```

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

## Testing

The Meadows compiler includes a comprehensive test suite covering all aspects of compilation:

### Running Tests

```bash
# Run all tests
./run_tests.sh

# Run specific test categories
./run_tests.sh build         # Build project only
./run_tests.sh test          # Run comprehensive test suite  
./run_tests.sh individual    # Test individual files
./run_tests.sh compile       # Test compilation to executable
./run_tests.sh comprehensive # Run all comprehensive categories
./run_tests.sh errors        # Test error handling
./run_tests.sh performance   # Performance benchmarks
./run_tests.sh applications  # Real-world application tests
```

All tests generate an HTML report in `build/test_report.html` showing detailed results.

### Test Coverage

- **Language Features**: Lexer, parser, expressions, statements, functions, classes
- **Compilation**: End-to-end compilation to executable files
- **Error Handling**: Syntax errors, runtime errors, edge cases, graceful recovery
- **Performance**: Compilation speed, memory usage, stress testing with large programs
- **IR Generation**: LLVM code quality, optimization detection
- **Integration**: Full pipeline testing from source to executable
- **Applications**: Real-world examples (calculator, data structures, algorithms)

### Test Files

- `tests/test_*.py` - Comprehensive test programs covering all language features
- Built-in C++ test framework validates parsing, compilation, and execution
- Automated performance benchmarking and regression detection
