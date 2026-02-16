# Meadows Language Roadmap

## Core Language (ESSENTIAL)

Basic features needed for a functional programming language to work.

- [x] Type system with inference
- [x] Module system
- [x] Functions and function calls
- [x] Control flow (if/else, while, for, break/continue)
- [x] Expressions and statements
- [x] Operators and literals
- [x] Comments

---

## Data Types & Collections (ESSENTIAL)

Data structures needed to build real programs.

- [x] Primitive types (int, float, bool, string)
- [x] Arrays
- [x] Structs
- [x] Enums with variants
- [x] Generic collections (Vec, HashMap, HashSet)
- [x] Option/Result types for error handling

---

## Pattern Matching (ESSENTIAL)

Important for control flow and data modeling.

- [x] Enums and tagged unions (with variants)
- [x] Match expressions (select-based codegen)
- [x] Basic pattern matching (literals, wildcards, bindings)
- [x] Nested if-else codegen fixes
- [ ] Exhaustive match checking
- [ ] Advanced pattern destructuring (tuple, struct patterns)

---

## Standard Library (ESSENTIAL)

Core utilities that every program needs.

### Implemented
- [x] std.string, std.math, std.io, std.os, std.time
- [x] std.array, std.vec, std.hashmap, std.hashset
- [x] std.option, std.result, std.panic
- [x] std.dir, std.args

### Planned
- [ ] std.json, std.regex
- [ ] std.http, std.net
- [ ] std.crypto, std.fs, std.process
- [ ] std.thread, std.sync, std.collections

---

## Tooling (IMPORTANT)

Developer experience improvements.

- [x] Code formatter (clang-format integration via `scripts/dev/format.sh`)
- [x] Static analysis (clang-tidy/cppcheck via `scripts/dev/run_static_analysis.sh`)
- [ ] Built-in linter (`--lint` flag)
- [ ] Documentation generator (`meadows doc`)
- [ ] Debugger support (DWARF generation)

---

## Advanced Type System (NON-ESSENTIAL)

Advanced features for more expressive type modeling.

- [ ] Monomorphization
- [ ] Type constraints
- [ ] Trait system
- [ ] Trait objects

---

## Memory & Concurrency (NON-ESSENTIAL)

Advanced features for safe concurrent programming.

- [ ] Ownership system
- [ ] Borrow checker
- [ ] Lifetime tracking
- [ ] Async/await
- [ ] Task spawning
- [ ] Channels and synchronization

---

## Metaprogramming (NON-ESSENTIAL)

Advanced features for code generation and abstraction.

- [ ] Macros
- [ ] Closures and iterators

---

## Implementation Status

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
- Built-in linter with `--lint` flag
- Documentation generator

### Version Milestones

| Version | Focus | Status |
|---------|-------|--------|
| v0.4 | Core Language | Complete |
| v0.5 | Pattern Matching | Complete |
| v0.6 | Tooling | In Progress |
| v0.7 | Generics | Planned |
| v0.8 | Memory Safety | Planned |
| v0.9 | Concurrency | Planned |
| v1.0 | Stable Release | Planned |

---

## Goals for v1.0 (Stable Release)

- [ ] Full standard library
- [ ] Performance optimization
- [ ] Cross-compilation
