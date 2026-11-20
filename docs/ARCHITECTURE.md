# Architecture

One front end feeds two back ends, and both are held to the same specification.

```
source ─ Lexer ─ Parser ─ ModuleLoader ─ SemanticAnalyzer ─┬─ BytecodeCompiler ─ Interpreter   (CLI --run, browser)
                                                           └─ CodeGen ─ LLVM IR ─ clang + runtime   (native)
```

## Front end (`src/lexer`, `src/parser`, `src/loader`, `src/sema`)

- **Lexer** turns a file into tokens. Invalid input is reported and skipped;
  lexical errors stop the pipeline before parsing so they do not cascade.
- **Parser** is recursive descent with statement-level error recovery, so one
  run reports many syntax errors. Statement nesting and expression height are
  capped at 256, which bounds the recursion of every later pass.
- **ModuleLoader** parses each file separately and splices an imported file's
  statements in place of its `import`. Diagnostics keep the file they came from.
- **SemanticAnalyzer** resolves names, checks arities and control-flow
  placement, and emits warnings. Functions are closed, so a name resolves to
  exactly one of a local, a function or a built-in.

All stages report through `DiagnosticSink` (`src/utils`), which owns the warning
policy and caps output at 100 diagnostics. `SourceMap` owns file contents so
diagnostics can show source lines without touching the file system.

## Interpreter (`src/interpreter`)

`BytecodeCompiler` lowers the AST to a stack-machine program: locals live in
frame slots chosen at compile time, scopes clear their slots on exit, and
control flow is jumps. `Interpreter` runs it on an explicit frame stack. Because
calls never recurse on the host stack, the call-depth limit is exact in the CLI
and in WebAssembly, where the engine's own stack is small. `Value` is a variant
with reference-counted strings, arrays and objects; deep structures are freed
iteratively.

## Native back end (`src/codegen`, `src/runtime`)

`CodeGen` emits LLVM IR that calls `meadows_rt.c`, a small C runtime that
defines the value representation, reference counting and every operation.
Generated code keeps one slot per expression, passes ownership explicitly
(every expression yields an owned value that its consumer releases) and releases
locals on every path out of a scope. The runtime is embedded in the compiler at
build time; `Meadows` writes it next to the IR and compiles both with `clang`.
The program runs on a 256 MiB thread so deep recursion behaves like the
interpreter.

Both back ends read their limits from `src/runtime/Limits.h` and format runtime
errors identically, including the source location of the failing operation.

## Browser playground (`src/wasm`, `web`)

`WasmBridge.cpp` exposes `analyze` (diagnostics, tokens, syntax tree and the
interpreter's bytecode listing) and `execute` through Embind. The files of a
project are written into the module's in-memory file system, so `import` works
as it does on disk. Output reaches JavaScript as raw bytes, which keeps embedded
NUL bytes intact. In the page, each run is a fresh Web Worker, so it can be
stopped and cannot block the interface; a long-lived worker handles analysis for
inline diagnostics.

## Editor support (`lsp`)

`Meadows --lsp-diagnostics --stdin <path>` checks an unsaved buffer as if it were
saved at `<path>` and prints JSON. The language server and the VS Code extension
are TypeScript projects that call it.

## Keeping the back ends aligned

`tests/conformance` holds programs with their exact expected output and error
text. It runs against the interpreter, the native compiler and the WebAssembly
build. `tests/fuzz/differential.py` generates random programs and requires the
interpreter and the compiled program to agree.
