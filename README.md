# Meadows Compiler

A simple compiled programming language called "Meadows" that compiles to LLVM IR and then to native executables.

## Features

- **Variables**: `let name = "Alice"; let age = 30;`
- **Functions**: `func greet(person) { print "Hello, " + person; }`
- **Conditionals**: `if (age > 18) { print "Adult"; } else { print "Minor"; }`
- **Loops**: `for (i in range(0, 5)) { print i; }` and `while` loops
- **Comments**: `# comment` or `// comment`
- **Print**: `print "Hello, world";`

## Building

### Local Build

```bash
./build.sh
```

This will create the `meadows` compiler in the `build/` directory.

### Pre-built Binaries

Pre-built binaries for Linux and macOS are available in [GitHub Releases](https://github.com/yourusername/meadows/releases). Download the appropriate archive for your platform and run the included `install.sh` script.

## Usage

Compile a `.ms` file:

```bash
./build/meadows path/to/file.ms
```

This generates `file.ms.ll` (LLVM IR) and `file.ms.out` (executable).

Run the executable:

```bash
./file.ms.out
```

## Testing

Run the test suite:

```bash
./test.sh
```

This will build the project and run all tests in the `tests/` directory, comparing outputs to expected results. See [TESTING.md](TESTING.md) for details on the test framework.

## Releases

Automated releases are created when version tags (e.g., `v1.0.0`) are pushed to the repository. Each release includes:

- Pre-built binaries for Linux x64 and macOS x64
- Debug and release builds
- Installation scripts
- Documentation

See [GitHub Actions workflow](.github/workflows/release.yml) for details.

## Project Structure

- `src/lexer/`: Lexical analysis
- `src/parser/`: Syntax parsing and AST construction
- `src/ast/`: Abstract syntax tree definitions
- `src/codegen/`: LLVM IR code generation
- `src/main/`: Compiler entry point
- `tests/`: Test cases (.ms files and .expected files)
- `CMakeLists.txt`: Build configuration
- `build.sh`: Build script
- `test.sh`: Test runner
- `format.sh`: Code formatting script (placeholder)
- `.github/workflows/`: GitHub Actions automation

## Dependencies

- CMake
- LLVM (installed via Homebrew on macOS)
- clang++ (for final compilation to executable)

## Language Syntax

Statements end with `;`, blocks use `{ }`, whitespace is ignored.

Example program:

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