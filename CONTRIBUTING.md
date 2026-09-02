# Contributing to Meadows

## Quick start

```bash
git clone https://github.com/nandan-varma/meadows
cd meadows
./build.sh debug          # build compiler + tests
./test.sh                 # run full test suite
```

## Workflow

1. Fork the repository and create a branch from `main`.
2. Make your changes.
3. Format: `./scripts/dev/format.sh`
4. Test: `./test.sh`
5. Open a pull request against `main`.

## Code style

- C++17, styled per `.clang-format` at the repo root (not currently CI-enforced —
  run `./scripts/dev/format.sh` before submitting).
- No raw owning pointers — use `std::unique_ptr`.
- New compiler passes must have unit tests under `tests/unit/`.
- New language features must have an integration test under `tests/integration/`,
  and should generally work in both backends (native CodeGen and the
  interpreter) — see [AGENTS.md](AGENTS.md#adding-ast-nodes) if you're
  adding a new AST node.

## Reporting bugs

Open an issue at <https://github.com/nandan-varma/meadows/issues>.
