# Changelog

## 2.0.0 — 2026-11-19

Meadows is now one language with three implementations that behave identically:
the native compiler, the interpreter and the browser playground. A conformance
suite and a differential fuzzer enforce it.

### Changed (breaking)

- **Native code has the language's full semantics.** Strings, floats, arrays,
  objects and functions can be passed to and returned from any function and
  stored anywhere; the earlier integer-only restrictions are gone.
- `push(array, value)` appends in place and returns the same array. `arr =
  push(arr, v)` still works.
- `&&` and `||` yield `0` or `1`.
- Functions are closed: they cannot read or write top-level variables (`E3001`).
- A name cannot be both a variable and a function or built-in (`E3013`).
- String literals cannot span lines, unknown escapes are errors (`E1003`),
  integer literals must fit in 32 bits (`E1004`), and unexpected characters are
  lexical errors (`E1002`) instead of becoming identifiers.
- Duplicate fields in an object literal are an error (`E3014`); objects keep
  their fields in source order and evaluate them in that order.
- Arrays, objects and functions compare by identity.
- Runtime errors go to standard error with a source location and exit status `1`.
- Command line: `-Wall` and `-Wextra` are removed; use `-Werror` and
  `-Wno-<name>`. The default output is the source name without extension.
  Windows is no longer supported.

### Added

- `Meadows --run` interprets a program without a C toolchain.
- `--dump-bytecode` prints the interpreter's bytecode listing.
- `else if`, calls on any expression (`f(1)(2)`, `fs[0](x)`), and mixed
  indexing, field access and calls in one chain (`a[0].b[1]`).
- Reference counting in compiled programs with exact reclamation of acyclic data.
- A 10,000-call depth limit that is identical on every back end, including
  WebAssembly.
- Source locations on runtime errors, and diagnostics with source context,
  sorted by position, reported across all imported files.
- A browser playground with multi-file projects and imports, a command palette,
  completion and hover documentation, go to definition, formatting, Vim keys,
  clickable errors, token, syntax tree and bytecode views, a language reference,
  run on change, benchmarking, themes, zip export, embeddable share links and
  offline support.
- `--lsp-diagnostics --stdin` for analysing unsaved editor buffers at their real
  path.
- `MEADOWS_CFLAGS` to add flags when building compiled programs.

### Fixed

- Compiled programs that accumulated strings in a loop, called functions with
  non-integer arguments, or read `a[i].field` produced invalid IR or failed to
  parse.
- Unspecified object field order and evaluation order.
- Stack overflows on deeply nested input and on freeing long linked structures.
- Integer overflow cases in division and remainder (`INT_MIN / -1`).

## Earlier releases

Versions 1.0.0 to 1.0.2 are described in the git history.
