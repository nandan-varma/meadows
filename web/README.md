# Meadows Compiler - Web Interface

A WebAssembly-powered online IDE for the Meadows programming language. Write, compile, and execute Meadows code directly in your browser.

## Features

- **Real-time Compilation**: Compile Meadows code to LLVM IR using WebAssembly
- **Interactive Editor**: Syntax highlighting, line numbers, and auto-indentation
- **Multiple Output Views**: 
  - Execution results
  - Abstract Syntax Tree (AST)
  - Token analysis
  - LLVM IR code generation
- **Example Programs**: Pre-loaded examples to get started quickly
- **Responsive Design**: Works on desktop, tablet, and mobile devices

## Online Demo

Visit the online IDE at: [https://nandan-varma.github.io/meadows](https://nandan-varma.github.io/meadows)

## Local Development

### Prerequisites

To build the WebAssembly version locally, you need:

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
- CMake 3.15+
- LLVM (included with Emscripten)

### Building WebAssembly

1. **Install Emscripten SDK**:
   ```bash
   # Download and install
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```

2. **Build the WebAssembly module**:
   ```bash
   # From the project root
   ./build_wasm.sh build
   ```

3. **Start development server**:
   ```bash
   ./build_wasm.sh serve
   ```

4. **Open in browser**:
   Navigate to `http://localhost:8000`

### Build Commands

The `build_wasm.sh` script supports several commands:

```bash
./build_wasm.sh build    # Build WebAssembly module
./build_wasm.sh clean    # Clean previous builds
./build_wasm.sh test     # Build and test module
./build_wasm.sh serve    # Build and start development server
./build_wasm.sh help     # Show help information
```

### Manual Build

If you prefer to build manually:

```bash
# Configure with Emscripten
emcmake cmake -B build-wasm -S . -DBUILD_WASM=ON

# Build
emmake make -C build-wasm meadows_wasm

# Serve
cd web && python3 -m http.server 8000
```

## File Structure

```
web/
├── index.html          # Main HTML interface
├── styles.css          # CSS styling
├── app.js              # JavaScript application logic
├── pre.js              # WebAssembly pre-load script
├── meadows.js          # Generated WebAssembly JavaScript (after build)
└── meadows.wasm        # Generated WebAssembly binary (after build)
```

## Language Features

The Meadows language supports:

### Basic Syntax
```python
# Variables and arithmetic
x = 10
y = 20
result = x + y * 2

# Print output
print("Result:", result)
```

### Functions
```python
def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

print(factorial(5))  # Output: 120
```

### Control Flow
```python
# If statements
if x > 10:
    print("x is greater than 10")
else:
    print("x is not greater than 10")

# While loops
i = 0
while i < 5:
    print(i)
    i = i + 1
```

### Data Types
- **Integers**: `42`, `-17`
- **Floats**: `3.14`, `-2.5`
- **Strings**: `"Hello, World!"`
- **Booleans**: `True`, `False`

### Built-in Functions
- `print()`: Output to console

## Architecture

### WebAssembly Integration

The Meadows compiler is compiled to WebAssembly using Emscripten, providing:

- **Native Performance**: Near-native execution speed in the browser
- **Memory Safety**: WebAssembly's sandboxed execution environment
- **Cross-Platform**: Runs in any modern web browser
- **Offline Capability**: No server required for compilation

### Code Generation Pipeline

1. **Lexical Analysis**: Source code → Tokens
2. **Parsing**: Tokens → Abstract Syntax Tree (AST)
3. **Code Generation**: AST → LLVM IR
4. **Execution**: LLVM IR → Results (simulated)

## Browser Compatibility

The web interface supports:

- **Chrome**: Version 69+
- **Firefox**: Version 62+
- **Safari**: Version 14+
- **Edge**: Version 79+

WebAssembly is required for full functionality.

## Deployment

### GitHub Pages

The project includes GitHub Actions for automatic deployment:

1. **Continuous Integration**: Tests on multiple platforms
2. **WebAssembly Build**: Compiles to WASM on every push
3. **GitHub Pages Deployment**: Automatic deployment to GitHub Pages

### Manual Deployment

To deploy to any static hosting service:

1. Build the WebAssembly module:
   ```bash
   ./build_wasm.sh build
   ```

2. Deploy the `web/` directory contents to your hosting service

## Development

### Adding New Features

1. **Language Features**: Modify the compiler source code
2. **Web Interface**: Edit `app.js`, `styles.css`, or `index.html`
3. **Examples**: Add new examples to the `examples` object in `app.js`

### Testing

Run the test suite:

```bash
# Test native compiler
./run_tests.sh test

# Test WebAssembly build
./build_wasm.sh test
```

### Debugging

Enable debug mode by modifying the Emscripten flags in `CMakeLists.txt`:

```cmake
set_target_properties(meadows_wasm PROPERTIES
    COMPILE_FLAGS "-g -O0 -s ASSERTIONS=1"
    LINK_FLAGS "-g -O0 -s ASSERTIONS=1 -s SAFE_HEAP=1"
)
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test locally with `./build_wasm.sh test`
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details.

## Acknowledgments

- **LLVM Project**: For the compiler infrastructure
- **Emscripten**: For WebAssembly compilation toolchain
- **GitHub Actions**: For CI/CD pipeline
- **Modern Web Standards**: For making this possible in the browser
