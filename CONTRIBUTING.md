# Contributing

## Build and test

```bash
cmake -B build -DLLVM_DIR=<llvm-17>/lib/cmake/llvm
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

[docs/TESTING.md](docs/TESTING.md) describes every suite.

## Workflow

1. Branch from `main`.
2. Make one logical change per commit, with an imperative subject of at most 50
   characters.
3. Run `scripts/format.sh` (clang-format 17) and the full test suite.
4. Open a pull request. CI runs native, sanitizer, format, WebAssembly,
   language-server and CodeQL jobs.

## Standards

- C++17 in the style of `.clang-format`; the C runtime is C11.
- Code explains itself; avoid comments that restate it.
- Own memory with smart pointers and standard containers.
- A language change updates both back ends in the same commit, adds conformance
  programs with hand-checked expectations, and updates
  [docs/LANGUAGE.md](docs/LANGUAGE.md). The interpreter, the native compiler and
  the playground must stay identical.
- Report problems through diagnostics, not exceptions, wherever the pipeline can
  continue.

## Reporting bugs

Open an issue at <https://github.com/nandan-varma/meadows/issues> with a
minimal program, the command you ran and the output you expected.
