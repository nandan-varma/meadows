# Changelog

## [1.0.2] — 2025

### Added
- `import "path.ms";` (CLI only) — the fourth "new language feature": a
  minimal, lean multi-file module system. `ModuleResolver` textually splices
  an imported file's tokens in place before parsing, so the parser,
  semantic analyzer, and CodeGen are all unmodified — none of them need to
  know imports exist. No namespacing: an imported file's declarations land
  directly in the importing file's global scope. A file already resolved
  earlier in the same import graph is skipped on a later import of it
  (first-import-wins), which also makes circular imports terminate rather
  than looping forever. Import paths go through the same validation as the
  CLI's entry file (now shared via a new `utils/PathValidation.h`, rather
  than duplicating the checks): `.ms` extension, no `..` traversal, size
  limit. Not available in the browser playground — no filesystem to import
  from there; `import` is a plain parse error in that context.
- First-class function references, interpreter-only: `let f = add; f(1, 2);`
  — a bare function name is now a `Value` (`Value::Kind::Function`), so it
  can be bound to a variable, reassigned, passed as an argument to another
  function, and called through whatever variable currently holds it.
  `SemanticAnalyzer::visitCallExpr` now accepts a call through a local
  variable (deferring argument-count validation to a runtime check in
  `Interpreter::callFunction`, since the variable's target function isn't
  known statically); `CodeGen` still rejects it at compile time with a
  clear error, not a silent miscompile, since the native backend has no
  function-pointer type. Not full closures — Meadows functions are
  flat/global, so there's no enclosing scope for one to capture.
- `push(arr, value)` in both backends — returns a new array one element
  longer (`arr = push(arr, value);` to keep using the same name). Arrays
  stay fixed-size once created; push allocates a new one rather than
  growing in place. Since it always grows by exactly one, and CodeGen
  already tracks each array-typed variable's length at compile time (see
  the field-access-through-a-variable and array-len() entries above), the
  result's length is knowable at compile time too — no runtime length
  header needed, keeping the array representation exactly as before.

### Fixed
- **Stack-overflow (SIGSEGV) on deeply nested expressions — a real DoS
  vector, not just a style issue.** The parser's recursion-depth guard
  (`MAX_PARSE_DEPTH = 100` in `parseAssignment`) was silently bypassed by
  five call sites that re-entered the precedence chain through
  `Parser::parseExpr()`, which had no depth parameter and always restarted
  the count at 0: array-literal elements, object-literal values,
  parenthesized sub-expressions, index expressions, and call arguments.
  Nesting any of those constructs ~sufficiently deep (confirmed with
  100,000 nested parens) crashed the process with exit code 139 instead of
  reporting a parse error. Separately, `parseUnary` recursed on repeated
  `-`/`!` without incrementing `depth` at all, so a long chain of unary
  operators bypassed the guard by a different route. Both are fixed:
  `parseExpr` now threads an explicit `depth` through every nested call
  site, and `parseUnary` has its own depth check. Exceeding the limit now
  throws a clean `E2009 PARSE_UNEXPECTED_TOKEN "Expression nesting too
  deep"` unconditionally (even in diagnostics/LSP mode, unlike ordinary
  recoverable syntax errors) so the guard can't degrade into a cascade of
  unrelated follow-on diagnostics.
- **Array bounds checking was comparing the index against the array's
  first element's *value*, not its length.** `CodeGen::validateArrayBounds`
  loaded the first i32 at the array's address expecting a runtime length
  header that was never actually written there (array literals have always
  been plain element storage, no header) — so `arr[i]` was bounds-checked
  against `arr[0]`'s value. This went unnoticed because every existing
  test's first element happened to be numerically larger than any tested
  index, e.g. `[100, 2, 3, 4, 500]` masks the bug for any index 0–4. Adding
  `push()` surfaced it immediately (`push([1,2,3], 4)` has a first element
  of `1`, rejecting index 3 as "out of bounds" on a valid 4-element array).
  Fixed by passing the compile-time-known length directly instead of
  reading it from memory.
- Floats (`f64`, IEEE 754 double) in both backends: literals (`3.14`),
  arithmetic (`+ - * / %`), comparisons, `print`/`str`, and constant string
  concatenation. `1 + 2.5` promotes the Int operand and yields `3.5`; `1 ==
  1.0` is true. Deliberately scoped to arithmetic and comparisons — `!`,
  `&&`, `||`, and `if`/`while` conditions stay Int-only in both backends,
  since CodeGen's CreateCondBr/PHI-based logical-operator lowering is
  structurally tied to i32/i1 and extending it is a separate, larger change
  than "add a numeric type." `print`/`str`/string-concat all format floats
  with `%g` in both backends (CodeGen via printf/snprintf, the interpreter
  via snprintf into Value::displayString), so output is byte-identical
  between the compiled binary and the browser playground.
- `LiteralExpr` gained an explicit `LiteralKind` (Int/Float/Str) field, set
  by the parser from the token's actual content. Previously CodeGen and the
  Interpreter each separately guessed a literal's kind by sniffing whether
  its first character was a digit — the same fragile heuristic in two
  places, now a single source of truth.
- Fixed a pre-existing bug found while extending string concatenation to
  floats: concatenating a negative constant-integer literal
  (`"n=" + (0-5)`) used `ConstantInt::getZExtValue()`, which reads a
  negative i32's two's-complement bit pattern as a huge unsigned number
  (`"n=4294967291"` instead of `"n=-5"`). Now uses `getSExtValue()`.
- Array element and object field assignment (`arr[0] = x`, `obj.field = x`)
  in both backends. `AssignExpr` generalized from a bare variable-name
  target to any of VarExpr/IndexExpr/FieldAccessExpr — the parser accepts
  exactly those three as assignment targets (`E2010
  PARSE_INVALID_ASSIGNMENT_TARGET` otherwise), and CodeGen's field-shape
  resolution (added for read access above) is shared between the read and
  write paths via `CodeGen::resolveFieldAccess` so they can't drift. An
  assignment expression evaluates to the assigned value, e.g.
  `print(arr[0] = 5);` is valid.
- Native LLVM backend: field access now resolves through a variable declared
  directly from an object literal (`let o = {a: 1}; let p = o; p.a`), not
  just an inline literal at the access site — shape metadata threads through
  chains of `let b = a;` aliasing. An unknown field, or a field-access target
  the compiler can't resolve a shape for (e.g. chained access `a.b.c`), is
  now a clear compile-time error instead of silently reading from the wrong
  memory. `CodeGen::visitObjectExpr` also now evaluates each field
  initializer exactly once (previously twice, for any object literal).
- Native LLVM backend: `len()` now works on arrays, not just strings —
  resolved as a compile-time constant from the array literal or the variable
  (including aliases) it came from, since arrays are fixed-size.
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
