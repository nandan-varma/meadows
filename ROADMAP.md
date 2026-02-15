# Meadows Language Roadmap

## Milestones

### v0.4 - Core Language
Core compiler with essential features for a functional programming language.

- [x] Type system with inference
- [x] Error handling with Option/Result types
- [x] Generic collections (Vec, HashMap, HashSet)
- [x] Module system

### v0.5 - Pattern Matching ✅ COMPLETE
Advanced control flow and data modeling.

- [x] Enums and tagged unions (with variants)
- [x] Match expressions (select-based codegen)
- [x] Basic pattern matching (literals, wildcards, bindings)
- [x] Nested if-else codegen fixes

**Status:** Released - All features implemented and tested

### v0.6 - Tooling 🚧 IN PROGRESS
Developer experience improvements.

- [x] Code formatter (clang-format integration via `scripts/dev/format.sh`)
- [x] Static analysis (clang-tidy/cppcheck via `scripts/dev/run_static_analysis.sh`)
- [ ] Built-in linter (`--lint` flag)
- [ ] Documentation generator (`meadows doc`)
- [ ] Debugger support (DWARF generation)

**Status:** Partial - External tools integrated, built-in tooling in development

### v0.7 - Generics
Advanced type system features.

- [ ] Monomorphization
- [ ] Type constraints
- [ ] Trait system

### v0.8 - Memory Safety
Rust-like ownership and borrowing.

- [ ] Ownership system
- [ ] Borrow checker
- [ ] Lifetime tracking

### v0.9 - Concurrency
Async runtime and parallelism.

- [ ] Async/await
- [ ] Task spawning
- [ ] Channels and synchronization

### v1.0 - Stable Release
Production-ready compiler.

- [ ] Full standard library
- [ ] Performance optimization
- [ ] Cross-compilation

---

## Standard Library

### Implemented
- std.string, std.math, std.io, std.os, std.time
- std.array, std.vec, std.hashmap, std.hashset
- std.option, std.result, std.panic
- std.dir, std.args

### Planned
- std.json, std.regex, std.http, std.net
- std.crypto, std.fs, std.process
- std.thread, std.sync, std.collections

---

## Language Features

### Implemented ✅
- **Lexer:** Keywords, operators, literals, comments, all token types
- **Parser:** Expressions, statements, functions, control flow, pattern matching
- **Type System:** Primitives, arrays, functions, structs, enums with variants
- **Code Generation:** LLVM IR, FFI, error handling, runtime validation
- **Pattern Matching:** Enums, match expressions, basic patterns (literals, wildcards)
- **Standard Library:** Collections (Vec, HashMap, HashSet), Option/Result types
- **Module System:** Imports, exports, module declarations
- **Control Flow:** If/else, while, for loops, break/continue
- **Development Tools:** Code formatting, static analysis scripts

### In Progress 🚧
- Exhaustive match checking
- Advanced pattern destructuring (tuple, struct patterns)
- Built-in linter with `--lint` flag
- Documentation generator

### Planned 📋
- Traits and trait objects
- Macros
- Closures and iterators
- Ownership system (v0.8)
- Async/await (v0.9)
