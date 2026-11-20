# Guidelines for coding agents

Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) first. The rule that shapes
everything: **the interpreter, the native compiler and the browser playground
implement one language.** If a change alters behavior, change both back ends and
the conformance suite together.

## Commands

```bash
cmake -B build -DLLVM_DIR=<llvm-17>/lib/cmake/llvm && cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/tests/meadows_tests "[parser]"        # one tag
scripts/format.sh                              # clang-format 17
```

## Style

- C++17, 2-space indent, 80 columns, `int* ptr`. The C runtime is C11.
- Naming: `PascalCase` types and files, `camelCase` functions and variables,
  `UPPER_SNAKE_CASE` constants, trailing underscore on private members.
- Smart pointers and standard containers; no raw `new` or `delete`.
- No comments that restate the code; prefer clearer names.
- Keep a name meaning one thing. Functions are closed over their own scope.

## Layout

```
src/utils       diagnostics, error codes, source map, path validation
src/lexer       tokens
src/parser      syntax and error recovery
src/loader      imports
src/sema        name resolution and checks
src/interpreter bytecode compiler and VM
src/runtime     C runtime for compiled programs; Limits.h is shared
src/codegen     LLVM IR generation and the clang step
src/driver      front-end pipeline shared by the CLI, tests and WebAssembly
src/main        command line
src/wasm        browser bridge
web             playground; lsp: language server and VS Code extension
tests           unit tests, conformance programs, fuzzer
```

## Adding a language feature

1. Lexer and parser, with an AST node if needed (`src/ast/AST.h`).
2. Semantic checks and, for a new diagnostic, an entry in `ErrorCodes.h`.
3. Both back ends: `BytecodeCompiler` and `Interpreter`, and `CodeGen` with
   `meadows_rt.c`. Share limits through `Limits.h`.
4. Conformance programs with hand-checked expectations, unit tests, and
   `docs/LANGUAGE.md`.
5. Run the differential fuzzer: `python3 tests/fuzz/differential.py build/bin/Meadows`.

## Errors

Report compile-time problems through `DiagnosticSink` so one run shows many.
Runtime errors use one message and one source location in both back ends.

## Git

Imperative subjects of at most 50 characters, one logical change per commit,
tests passing at each commit. Do not commit build output.
