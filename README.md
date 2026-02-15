# Meadows Programming Language

A systems programming language designed for safety, performance, and developer productivity. Compiles to native executables via LLVM.

**Current Status:** Alpha (25-30% production-ready) | **Target:** v1.0 Production Release

## Quick Start

```bash
# Build the compiler
./build.sh release

# Compile and run a Meadows program
./build/bin/Meadows hello.ms
./hello.ms.out

# Run tests
./test.sh
```

## What Makes Meadows Different?

Meadows combines the safety guarantees of modern languages with the performance of systems programming:

## Installation

### macOS
```bash
brew install meadows-lang
```

### Linux
```bash
curl -fsSL https://meadows-lang.org/install.sh | sh
```

### From Source
```bash
git clone https://github.com/meadows-lang/meadows.git
cd meadows
./build.sh release
sudo cp build/bin/Meadows /usr/local/bin/
```

## Language Features

### Basic Syntax
```meadows
# Variables and types
let name = "Alice";
let age: i32 = 30;
let pi: f64 = 3.14159;
let is_valid = true;

# Functions
func greet(person: string) -> string {
    return "Hello, " + person;
}

# Control flow
if (age >= 18) {
    print("Adult");
} else {
    print("Minor");
}

# Loops
for (i in range(0, 5)) {
    print(i);  # 0, 1, 2, 3, 4
}

while (count > 0) {
    count = count - 1;
}
```

### Structs and Enums ✅
```meadows
type Point = {
    x: i32,
    y: i32,
};

enum Status = {
    Pending,
    Success(string),
    Error { code: i32, message: string },
};

# Pattern matching
match (value) {
    0 => "zero",
    _ => "other",
};
```

### Generics 🚧
```meadows
# Basic generics (partially implemented)
func identity<T>(value: T) -> T {
    return value;
}

# Generic collections (available in stdlib)
let vec = Vec<i32>::new();
let map = HashMap<string, i32>::new();

# Full generic structs (planned for v0.7)
# struct Box<T> { value: T }
```

### Error Handling 🚧
```meadows
# Option and Result types (available)
import std.option;
import std.result;

func find_element(arr: Vec<i32>, target: i32) -> Option<i32> {
    for (item in arr) {
        if (item == target) {
            return Some(item);
        }
    }
    return None;
}

# Pattern matching on results
match result {
    Ok(value) => print(value),
    Err(e) => print("Error"),
};

# ? operator (planned for v0.7)
# let content = read_file("data.txt")?;
```

### Async/Await 📋 (Planned v0.9)
```meadows
# Async/await is planned for v0.9
# Current status: Not yet implemented

# Planned syntax:
# async func fetch_data(url: string) -> Result<Response, Error> {
#     let response = http_get(url).await?;
#     return Ok(response);
# }
```

## Standard Library

```meadows
import std.collections;
import std.io;
import std.string;

func main() -> i32 {
    # Dynamic arrays
    let vec = Vec<i32>::new();
    vec.push(1);
    vec.push(2);
    
    # Hash maps
    let map = HashMap<string, i32>::new();
    map.insert("key", 42);
    
    # File I/O
    let content = File::read("input.txt")?;
    File::write("output.txt", content)?;
    
    return 0;
}
```

## Package Manager

```bash
# Initialize a new project
meadows init my-project
cd my-project

# Add dependencies
meadows add serde
meadows add tokio --version "1.0"

# Build
meadows build

# Run tests
meadows test

# Format code
meadows fmt
```

## Roadmap to v1.0

### Phase 1: Foundation (Months 1-4) - Core Language
**Goal:** Complete type system and basic tooling

- [ ] **Type System Completion**
  - [ ] Generic type code generation (monomorphization)
  - [ ] Struct code generation (LLVM struct types)
  - [ ] Enum code generation (tagged unions)
  - [ ] Integrate TypeChecker into compiler pipeline
  
- [ ] **Multi-file Module Compilation**
  - [ ] Cross-module type checking
  - [ ] Separate compilation of modules
  - [ ] Incremental builds
  
- [ ] **Package Manager (MVP)**
  - [ ] Fetch dependencies from git repositories
  - [ ] Semantic versioning support
  - [ ] Lock file for reproducible builds
  - [ ] Local cache management
  
- [ ] **Testing Framework**
  - [ ] `meadows test` command
  - [ ] Built-in assert macros (`assert_eq`, `assert_panics`)
  - [ ] Test organization (test modules)
  - [ ] Coverage reporting

**Deliverable:** Can write and publish small libraries with proper dependencies and tests

### Phase 2: Safety & Reliability (Months 5-8)
**Goal:** Memory safety and robust error handling

- [ ] **Memory Safety System**
  - [ ] Ownership and borrowing rules
  - [ ] Lifetime tracking
  - [ ] Borrow checker integration
  - [ ] Smart pointer types (`Box<T>`, `Rc<T>`, `Arc<T>`)
  - Alternative: Garbage collector (faster to implement, less control)
  
- [ ] **Error Handling**
  - [ ] `Result<T, E>` type
  - [ ] `Option<T>` type
  - [ ] `?` operator for error propagation
  - [ ] Panic and recover mechanisms
  - [ ] Custom error types
  
- [ ] **Standard Collections**
  - [ ] `HashMap<K, V>`
  - [ ] `Vec<T>` (dynamic array)
  - [ ] `HashSet<T>`
  - [ ] `LinkedList<T>`
  - [ ] Iterator protocol
  
- [ ] **Pattern Matching**
  - [ ] `match` expressions
  - [ ] Destructuring
  - [ ] Guards and bindings

**Deliverable:** Production-ready memory safety and error handling for systems programming

### Phase 3: Ecosystem (Months 9-12)
**Goal:** Developer tooling and interoperability

- [ ] **Tooling**
  - [ ] Code formatter (`meadows fmt`)
  - [ ] Linter (`meadows lint`)
  - [ ] Documentation generator (`meadows doc`)
  - [ ] Debugger support (DWARF generation)
  - [ ] IDE integration (autocomplete, goto definition)
  
- [ ] **JSON Support**
  - [ ] JSON parsing and serialization
  - [ ] Derive macros for structs/enums
  - [ ] Streaming parser for large files
  
- [ ] **Build System Improvements**
  - [ ] Build caching
  - [ ] Parallel builds
  - [ ] Cross-compilation support
  - [ ] Link-time optimization
  
- [ ] **FFI Enhancements**
  - [ ] Complex C struct support
  - [ ] Callback functions
  - [ ] Automatic binding generation

**Deliverable:** Professional development experience comparable to Go/Rust

### Phase 4: Performance & Scale (Months 13-18)
**Goal:** Concurrency, networking, and large-scale applications

- [ ] **Concurrency**
  - [ ] Async/await runtime
  - [ ] Tasks and executors
  - [ ] Channels for communication
  - [ ] Mutex and RwLock
  - [ ] Atomic operations
  
- [ ] **Networking Stack**
  - [ ] TCP/UDP sockets
  - [ ] HTTP client/server
  - [ ] TLS/SSL support
  - [ ] WebSocket support
  
- [ ] **WebAssembly Support**
  - [ ] Compile to WASM target
  - [ ] WASI support
  - [ ] Browser runtime
  
- [ ] **Performance**
  - [ ] Profile-guided optimization
  - [ ] SIMD intrinsics
  - [ ] Inline assembly
  - [ ] Zero-copy I/O

**Deliverable:** Can build high-performance web services and systems software

## Current Status

| Feature Category | Status | Completion |
|-----------------|--------|------------|
| **Type System** | Functional | 60% |
| **Pattern Matching** | Working | 85% |
| **Memory Management** | C-style | 30% |
| **Error Handling** | Option/Result | 50% |
| **Concurrency** | Not started | 0% |
| **Package Manager** | Config parsing | 30% |
| **Tooling** | Partial | 45% |
| **Standard Library** | Collections | 55% |
| **Documentation** | Good | 65% |

**Overall: ~25-30% of v1.0 complete**

## Comparison to Other Languages

| Language | Time to v1.0 | Meadows Status |
|----------|--------------|----------------|
| Go | 3 years (2009-2012) | Similar to 2009 |
| Rust | 5 years (2010-2015) | Similar to 2011 |
| Swift | 4 years (2010-2014) | Similar to 2011 |

**Estimated Meadows v1.0:** 18 months with focused development

## Contributing

We welcome contributions! Please see:
- [AGENTS.md](AGENTS.md) - Detailed guidelines for AI coding agents
- [CONTRIBUTING.md](CONTRIBUTING.md) - Human contributor guide
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - Architecture overview

### Priority Areas for Contribution

1. **Type System** - Help complete generic and struct code generation
2. **Standard Library** - Implement collections in Meadows
3. **Documentation** - Improve examples and tutorials
4. **Testing** - Add more test coverage

## Documentation

- [Language Reference](docs/LANGUAGE.md) - Complete language specification
- [Architecture Overview](docs/ARCHITECTURE.md) - Compiler internals
- [Testing Guide](docs/TESTING.md) - How to test Meadows code
- [API Reference](https://docs.meadows-lang.org) - Standard library docs

## Community

- **Discord:** [discord.gg/meadows](https://discord.gg/meadows)
- **Forum:** [forum.meadows-lang.org](https://forum.meadows-lang.org)
- **Twitter:** [@meadows_lang](https://twitter.com/meadows_lang)
- **Blog:** [blog.meadows-lang.org](https://blog.meadows-lang.org)

## License

Meadows is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Acknowledgments

Meadows is built on the shoulders of giants:
- **LLVM Project** - Code generation and optimization
- **Rust** - Ownership system inspiration
- **Go** - Simplicity and tooling philosophy
- **TypeScript** - Type system ideas
- **Swift** - Ergonomic syntax inspiration

---

**Ready to build the future of systems programming?** [Get started →](https://docs.meadows-lang.org/tutorial)
