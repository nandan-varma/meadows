# Changelog

## [1.0.2] — 2025

### Added
- `len(s)` built-in: returns the length of a string as i32
- `str(n)` built-in: formats an integer as a decimal string (runtime `snprintf`)
- Bare `return;` is now allowed in functions; returns 0
- Field access on object literals is statically validated; unknown fields
  produce `E3012 SEM_UNKNOWN_FIELD` at compile time

### Fixed
- `release.yml` was using a stale `steps.install-llvm.outputs.llvm-path`
  reference that no longer resolves; switched to `env.LLVM_PATH` to match
  `ci.yml`. Tag-triggered releases work again.

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
