# Changelog

## [1.0.2] — 2025

### Added
- Browser playground at meadows.nandanvarma.com: a WASM build (`-DBUILD_WASM=ON`,
  Emscripten) of the lexer/parser/semantic analyzer, plus a new tree-walking
  `Interpreter` (`src/interpreter/`) that actually runs programs client-side —
  the native pipeline's final step (shelling out to `clang++`) has no browser
  equivalent, so this is a second, independent backend, not a simulation.
  Deployed via `.github/workflows/deploy-playground.yml` on push to `main`.
- The interpreter implements the full dynamic language from `docs/LANGUAGE.md`
  and is a strict superset of the current LLVM backend where that backend
  narrows things for implementation reasons: array elements aren't
  i32-restricted, function parameters/returns accept any value kind, object
  field access works through a variable (not just inline literals), and
  `len()` supports arrays.
- Interpreter safety limits (step count, call depth, string length) since
  execution runs synchronously on the browser's main thread with no way to
  preempt a runaway script.
- `len(s)` built-in: returns the length of a string as i32
- `str(n)` built-in: formats an integer as a decimal string (runtime `snprintf`)
- Bare `return;` is now allowed in functions; returns 0
- Field access on object literals is statically validated; unknown fields
  produce `E3012 SEM_UNKNOWN_FIELD` at compile time

### Fixed
- `release.yml` was using a stale `steps.install-llvm.outputs.llvm-path`
  reference that no longer resolves; switched to `env.LLVM_PATH` to match
  `ci.yml`. Tag-triggered releases work again.
- CodeGen had no pre-pass declaring function signatures before generating
  bodies, so a function calling another function defined *later* in the file
  failed with "Undefined function" at codegen time — even though
  SemanticAnalyzer's own pre-pass had already accepted the program as valid.
  True mutual recursion between two top-level functions was broken
  regardless of definition order. Added `CodeGen::declareFunctionSignatures`.
- String `==`/`!=` compiled to a raw pointer comparison (`icmp eq` on the two
  `i8*` values), which only happened to be true when LLVM deduplicated two
  identical string-literal globals into the same constant — never true for
  two runtime-built strings, and not a property Meadows programs should have
  had to depend on. Now lowers to a `strcmp` call and compares content.

### Documentation
- `docs/LANGUAGE.md` rewritten for accuracy: documents `%`, `break`/`continue`,
  built-ins, error codes, and the current limitations honestly

## [1.0.1] — 2025

### Added
- `%` modulo operator
- `break` and `continue` in `for`/`while` loops (codegen fix — `continue` now correctly resumes at the increment step in `for` loops)
- Sanitizer CI job (ASan + UBSan) with `-fno-sanitize=vptr` for LLVM static-lib compatibility
- Negative compilation test infrastructure (`tests/integration/errors/`) — compiler is now verified to correctly reject invalid programs
- Semantic error messages now include the source filename

### Fixed
- Duplicate function definitions and duplicate variable declarations in the same scope are now detected and reported as errors (previously silently accepted, producing broken IR)
- `for` loop `continue` was jumping to the condition check instead of the increment step
- Double-terminator IR generated when `break` appeared at the end of a loop body

## [1.0.0] — 2025

### Initial release
- Lexer, parser, semantic analyzer, and LLVM 17 IR code generator
- Variables (`let`), functions (`func`), `if`/`else`, `for`/`while`, `return`/`break`/`continue`
- Arithmetic, comparison, logical operators
- Arrays, objects, field access, index expressions
- Built-in `print()`
- CLI: `--dump-ast`, `--dump-ir`, `-O0`–`-O3`, `-Wall`/`-Wextra`/`-Werror`/`-Wno-*`
- LSP diagnostics mode (`--lsp-diagnostics`)
- CI: build matrix (Linux + macOS), static analysis, CodeQL
