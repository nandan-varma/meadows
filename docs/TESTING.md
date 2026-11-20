# Testing

```bash
cmake -B build -DLLVM_DIR=<llvm-17>/lib/cmake/llvm
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Set `MEADOWS_CLANG` to the `clang` that should build compiled programs if the
default lookup (`clang-17`, `clang`) does not find the right one.

## Suites

| Suite                      | What it checks                                                      |
|----------------------------|---------------------------------------------------------------------|
| `meadows_tests`            | Unit tests (Catch2) for each stage: lexer, parser, loader, semantic analysis, interpreter, diagnostics, code generation |
| `conformance-interpreter`  | Every program in `tests/conformance` under `Meadows --run`          |
| `conformance-native`       | The same programs, compiled and executed                            |
| `conformance-diagnostics`  | Exact compile-time diagnostics                                       |
| `conformance-wasm`         | The same suite on the WebAssembly build (`BUILD_WASM`, needs Node)   |
| `examples-*`               | The programs in `examples/`, interpreted and compiled               |
| `differential-fuzz`        | Random programs must behave identically on both back ends           |

Run one unit test with `./build/tests/meadows_tests "<name>"` or by tag, for
example `"[parser]"`.

## Conformance programs

Each `tests/conformance/<name>.ms` has `<name>.expected` (standard output) and,
for programs that fail at run time, `<name>.expected_error` (standard error; exit
status is then `1`). Programs under `errors/` end in a runtime error, and
`diagnostics/` holds programs whose compile-time output is compared.

To record new expectations, run
`tests/run_conformance.sh build/bin/Meadows interpret --update`, then read every
changed file: the expected output is the specification, so check it by hand.
Because the suite also runs natively, a divergence between back ends fails one
of the two runs.

## WebAssembly and the playground

```bash
emcmake cmake -B build-wasm -DBUILD_WASM=ON
cmake --build build-wasm
ctest --test-dir build-wasm
cd web && npm ci && MEADOWS_WASM=../build-wasm/bin/meadows.js npm test && npm run build
npm run serve        # http://localhost:8080
```

## Sanitizers

```bash
cmake -B build-san -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize=vptr" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-san
MEADOWS_CFLAGS="-fsanitize=address,undefined -g" ctest --test-dir build-san
```

`MEADOWS_CFLAGS` adds flags to the `clang` step that builds compiled programs.

## Formatting

`scripts/format.sh` formats and `scripts/format.sh --check` verifies, using
clang-format 17 (`CLANG_FORMAT=<path>` selects the binary).
