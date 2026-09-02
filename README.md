# Meadows

Meadows is a small, C-like compiled programming language. It compiles `.ms`
source files to LLVM IR and then to a native executable via `clang++`.

Try it in the browser — no install required: **[meadows.nandanvarma.com](https://meadows.nandanvarma.com)**

```meadows
func fib(n) {
  if (n <= 1) { return n; }
  return fib(n - 1) + fib(n - 2);
}

for (i in range(0, 8)) {
  print(fib(i));
}
```

## Quick start

Requires CMake 3.16+, LLVM 17.x, and a C++17 compiler.

```bash
# macOS
brew install cmake llvm@17

# Linux (Debian/Ubuntu)
sudo apt-get install cmake llvm-17-dev clang
```

```bash
git clone https://github.com/nandan-varma/meadows.git
cd meadows
./build.sh release          # builds ./build-release/bin/Meadows

./build-release/bin/Meadows examples/hello.ms
./hello.out
```

If LLVM isn't on the default search path, point the build at it directly:

```bash
./build.sh release --llvm /opt/homebrew/opt/llvm@17/lib/cmake/llvm
```

## What's here

Meadows is more than just a compiler — this repo is the whole toolchain:

| Piece | Where | What it is |
|---|---|---|
| Compiler (native) | `src/` → `Meadows` binary | Lexer → Parser → Semantic Analyzer → LLVM CodeGen → `clang++` |
| Browser playground | `web/`, `src/wasm/`, `src/interpreter/` | Same front end compiled to WebAssembly, running a tree-walking interpreter client-side. Deployed at [meadows.nandanvarma.com](https://meadows.nandanvarma.com) |
| Language Server | `lsp/meadows-lsp/` | LSP implementation for editors, backed by the compiler's `--lsp-diagnostics` mode — see [`lsp/README.md`](lsp/README.md) |
| VS Code extension | `lsp/meadows-vscode/` | Syntax highlighting + the language server, packaged for VS Code |

The playground runs a **second backend** (an interpreter, not LLVM) because
there's no way to shell out to `clang++` inside a browser sandbox. It's kept
at feature parity with the native compiler; where they still differ, that's
documented in [`docs/LANGUAGE.md`](docs/LANGUAGE.md#current-limitations).

## The language

```meadows
let name = "Alice";
let age = 30;
let scores = [95, 88, 72];
let person = {name: "Bob", age: 25};

func greet(who) {
  print("Hello, " + who);
}

if (age >= 18) {
  greet(name);
} else {
  print("too young");
}

for (i in range(0, len(scores))) {
  print(scores[i]);
}
```

Variables, functions, `if`/`else`, `while`, range-based `for`, arrays,
objects, ints, floats, strings, and single-file `import`. Full reference,
including built-ins, error codes, and exact semantics:
**[docs/LANGUAGE.md](docs/LANGUAGE.md)**.

## Usage

```bash
Meadows <file.ms> [OPTIONS]

  -o, --output <path>   Output executable path (default: <name>.out)
  -O, --opt-level <0-3> Optimization level
  -v, --verbose         Show compilation phases and timing
  --dump-ast            Print the AST and exit
  --dump-ir             Print generated LLVM IR and exit
  --lsp-diagnostics     Emit diagnostics as JSON (used by the language server)
```

Compiling `path/to/file.ms` produces `path/to/file.ms.ll` (LLVM IR, deleted
after a successful build) and `file.out` (the executable, written to the
*current directory* — its name is the source file's stem, not its full
path), unless `-o` is given.

## Building and testing

```bash
./build.sh                    # build debug + release + tests
./build.sh debug               # build-debug/bin/Meadows
./build.sh release             # build-release/bin/Meadows
./build.sh tests               # build/tests/meadows_tests
./build.sh clean               # remove all build directories

./test.sh                      # run everything: unit + integration + security
./test.sh unit                 # unit tests only
./test.sh integration           # integration tests only (.ms programs)
```

`build.sh`/`test.sh` are thin wrappers; the project is plain CMake underneath:

```bash
cmake -S . -B build -DBUILD_TESTS=ON -DLLVM_DIR=<path-to-llvm-17-cmake-dir>
cmake --build build -j
ctest --test-dir build
```

See [docs/TESTING.md](docs/TESTING.md) for the full testing guide and
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for how the compiler is put
together internally.

## Project layout

```
meadows/
├── src/
│   ├── lexer/        Tokenizer + CLI-only multi-file import resolution
│   ├── parser/       Recursive-descent parser → AST
│   ├── ast/          AST node definitions (visitor pattern)
│   ├── sema/         Semantic analysis (name resolution, type/arity checks)
│   ├── codegen/      LLVM IR generation (native backend)
│   ├── interpreter/  Tree-walking interpreter (playground backend)
│   ├── wasm/         Emscripten/embind bridge for the playground
│   ├── lsp/          JSON diagnostics output for --lsp-diagnostics
│   ├── utils/        Diagnostics, error codes, exceptions, path validation
│   └── main/         CLI entry point
├── web/               Browser playground (static site, WebAssembly)
├── lsp/               Language Server (TypeScript) + VS Code extension
├── tests/             Unit (Catch2), integration (.ms), and security tests
├── examples/          Example .ms programs
├── docs/              Language reference, architecture, testing guide
└── scripts/           Build, test, and dev tooling
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Bug reports and security issues:
see [SECURITY.md](SECURITY.md) for the latter.

## License

MIT — see [LICENSE](LICENSE).
