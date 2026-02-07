# Meadows Compiler

A simple compiled programming language called "Meadows" that compiles to LLVM IR and then to native executables.

## Quick Start

```bash
# Build everything (compiler + tests)
./build.sh

# Run all tests
./test.sh

# Compile and run a Meadows program
./build/meadows hello.ms
./hello.ms.out
```

## Features

- **Variables**: `let name = "Alice"; let age = 30;`
- **Functions**: `func greet(person) { print "Hello, " + person; }`
- **Conditionals**: `if (age > 18) { print "Adult"; } else { print "Minor"; }`
- **Loops**: `for (i in range(0, 5)) { print i; }` and `while` loops
- **Comments**: `# comment` or `// comment`
- **Print**: `print "Hello, world";`

## Building

The project uses a modular build system. All build commands are available through the main `./build.sh` script.

```bash
./build.sh                    # Build everything (default)
./build.sh debug             # Build debug version only
./build.sh release           # Build release version only
./build.sh tests             # Build test executables
./build.sh clean             # Clean all build artifacts
```

### Build Options

```bash
./build.sh [TARGET] [OPTIONS]

Options:
  -j N         Use N parallel jobs (default: auto-detect)
  -v           Verbose output
  --llvm PATH  Specify LLVM cmake directory

Examples:
  ./build.sh release -j8              # Release build with 8 jobs
  ./build.sh tests --verbose          # Build tests verbosely
  ./build.sh debug --llvm /usr/lib/llvm-17/lib/cmake/llvm
```

### Build Outputs

- **Debug**: `build-debug/meadows`
- **Release**: `build-release/meadows`
- **Tests**: `build/tests/meadows_tests`

## Testing

Comprehensive test suite with unit tests, integration tests, and security tests.

```bash
./test.sh                    # Run all tests (default)
./test.sh unit              # Run unit tests only
./test.sh integration       # Run integration tests (.ms files)
./test.sh security          # Run security tests
./test.sh coverage          # Generate coverage report
```

### Test Options

```bash
./test.sh [SUITE] [OPTIONS]

Options:
  -v, --verbose    Verbose output
  -f, --fail-fast  Stop on first failure
  --no-build       Don't rebuild before testing

Examples:
  ./test.sh unit -v              # Verbose unit tests
  ./test.sh all -f               # All tests, fail fast
  ./test.sh integration --no-build  # Skip rebuild
```

### Test Coverage

```bash
./test.sh coverage
# Generates coverage_report/index.html
```

See [TESTING.md](TESTING.md) for detailed testing documentation.

## Usage

Compile a `.ms` file:

```bash
./build/meadows path/to/file.ms
```

This generates:
- `file.ms.ll` - LLVM IR
- `file.ms.out` - Native executable

Run the executable:

```bash
./file.ms.out
```

## Project Structure

```
meadows/
├── src/                    # Source code
│   ├── lexer/             # Lexical analysis
│   ├── parser/            # Syntax parsing
│   ├── ast/               # Abstract syntax tree
│   ├── codegen/           # LLVM IR generation
│   └── main/              # Compiler entry point
├── scripts/               # Build and development scripts
│   ├── build/             # Build scripts
│   ├── test/              # Test scripts
│   └── dev/               # Development utilities
├── tests/                 # Test cases
│   ├── *.ms               # Meadows test files
│   ├── *.expected         # Expected output files
│   └── unit/              # Unit tests (Catch2)
├── docs/                  # Documentation
├── build.sh               # Main build script
├── test.sh                # Main test script
└── CMakeLists.txt         # CMake configuration
```

## Dependencies

- **CMake** (3.10+)
- **LLVM** (17.x) - Install via Homebrew on macOS: `brew install llvm@17`
- **clang++** - For final compilation to executable
- **Python 3** - For some test utilities (optional)

### macOS Setup

```bash
brew install cmake llvm@17
```

### Linux Setup

```bash
sudo apt-get install cmake llvm-17-dev clang
```

## Development Scripts

Located in `scripts/` directory:

```bash
# Format code
./scripts/dev/format.sh           # Format all source files
./scripts/dev/format.sh --check   # Check formatting (CI mode)
```

## Language Syntax Quick Reference

Meadows is a C-style compiled language with simple syntax.

### Statements & Comments

```meadows
let x = 42;           # Variable declaration (ends with ;)
print "Hello";        # Print statement
// This is a comment
```

### Variables & Types

```meadows
let name = "Alice";        # String
let age = 30;              # Integer
let numbers = [1, 2, 3];   # Array
let person = {            # Object
    name: "Bob",
    age: 25
};
```

### Operators

```meadows
# Arithmetic
let sum = 10 + 5;         # Addition
let diff = 10 - 5;        # Subtraction
let prod = 10 * 5;        # Multiplication
let quot = 10 / 5;        # Division
let neg = -10;            # Unary minus

# Comparison
let eq = 10 == 10;        # Equal
let gt = 10 > 5;          # Greater than
let lt = 5 < 10;          # Less than
let ge = 10 >= 10;        # Greater or equal
let le = 5 <= 10;         # Less or equal
```

### Control Flow

```meadows
if (age > 18) {
    print "Adult";
} else {
    print "Minor";
}

while (count > 0) {
    print count;
    count = count - 1;
}

for (i in range(0, 5)) {
    print i;  # Prints 0, 1, 2, 3, 4
}
```

### Functions

```meadows
func factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

let result = factorial(5);
print result;
```

### String Escapes

```meadows
let newline = "line1\nline2";
let tab = "col1\tcol2";
let quote = "He said \"hello\"";
let path = "C:\\path\\to\\file";
```

### Complete Example

```meadows
func factorial(n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

let result = factorial(5);
print result;
```

## Testing Status

| Test Type | Count | Status |
|-----------|-------|--------|
| Unit Tests | 23 | ✅ Passing |
| Integration Tests | 4 | ✅ Passing |
| Security Tests | 12 | ✅ Passing |
| **Total** | **39** | **✅ All Pass** |

## Continuous Integration

GitHub Actions automatically builds and tests on every push:
- Multi-platform builds (Linux, macOS)
- Debug and release configurations
- Automated unit tests
- Integration tests
- Security tests

See [`.github/workflows/release.yml`](.github/workflows/release.yml) for details.

## Releases

Automated releases are created when version tags (e.g., `v1.0.0`) are pushed:
- Pre-built binaries for Linux x64 and macOS x64
- Debug and release builds
- Installation scripts

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests: `./test.sh`
5. Format code: `./scripts/dev/format.sh`
6. Submit a pull request

## Documentation

- [TESTING.md](TESTING.md) - Detailed testing guide
- [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md) - Implementation details
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - Architecture overview

## License

[Your License Here]

## Acknowledgments

Built with:
- LLVM Project
- Catch2 Testing Framework
- CMake Build System
