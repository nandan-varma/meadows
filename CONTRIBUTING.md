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

- C++17, clang-format enforced (`.clang-format` at root).
- No raw owning pointers — use `std::unique_ptr`.
- New compiler passes must have unit tests under `tests/unit/`.
- New language features must have an integration test under `tests/integration/`.

## Reporting bugs

Open an issue at <https://github.com/nandan-varma/meadows/issues>.
