# Meadows

A small, dynamically typed programming language. Meadows compiles to native
executables through LLVM, runs on a built-in interpreter, and runs in your
browser, all with identical behavior.

```meadows
func fib(n) {
  if (n < 2) { return n; }
  return fib(n - 1) + fib(n - 2);
}

for (i in range(0, 10)) {
  print("fib(" + i + ") = " + fib(i));
}
```

**[Try it in the browser →](https://meadows.nandanvarma.com)**

## Install

Meadows needs LLVM 17 to build and `clang` to produce executables.

```bash
brew install llvm@17            # macOS
sudo apt install llvm-17-dev clang-17   # Debian and Ubuntu

git clone https://github.com/nandan-varma/meadows
cd meadows
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The compiler is `build/bin/Meadows`. CMake finds Homebrew and Debian LLVM
installs automatically; otherwise pass `-DLLVM_DIR=<llvm>/lib/cmake/llvm`.

## Use

```bash
Meadows hello.ms              # compile to ./hello
Meadows hello.ms -o greet -O2 # name the output, optimize
Meadows --run hello.ms        # interpret without a C toolchain
Meadows --dump-ast hello.ms   # print the syntax tree
Meadows --dump-bytecode hello.ms # print the interpreter bytecode
Meadows --dump-ir hello.ms    # print the LLVM IR
```

| Option                   | Meaning                                                      |
|--------------------------|--------------------------------------------------------------|
| `-o, --output <path>`    | Executable to write (default: the source name without `.ms`) |
| `-O, --opt-level 0-3`    | Optimization level for the generated program                 |
| `--run`                  | Interpret instead of compiling                               |
| `--dump-ast`, `--dump-bytecode`, `--dump-ir` | Print the syntax tree, interpreter bytecode or LLVM IR and exit |
| `--lsp-diagnostics`      | Print diagnostics as JSON; with `--stdin`, read the source from standard input |
| `-Werror`, `-Wno-<name>` | Promote warnings to errors, or silence `unused-variable`, `unreachable-code`, `shadowing` |
| `-v, --verbose`          | Report each phase                                            |

Exit status is `0` on success and `1` for any compile error; a compiled program
exits `1` after a runtime error. Set `MEADOWS_CLANG` to choose the `clang` used
for linking and `MEADOWS_CFLAGS` to add flags to that step.

## The language

Integers, floats, strings, arrays, objects and first-class functions; `if`,
`while`, `for`, `break`, `continue`; imports; four built-ins. The
[language reference](docs/LANGUAGE.md) is the specification, and
[`examples/`](examples) has runnable programs.

## Project layout

| Path            | Contents                                                |
|-----------------|---------------------------------------------------------|
| `src/`          | Compiler, interpreter and C runtime                     |
| `web/`          | Browser playground                                      |
| `lsp/`          | Language server and VS Code extension                   |
| `tests/`        | Unit tests, conformance suite, fuzzer                   |
| `docs/`         | [Language](docs/LANGUAGE.md), [architecture](docs/ARCHITECTURE.md), [testing](docs/TESTING.md) |

See [CONTRIBUTING.md](CONTRIBUTING.md) to get involved and
[CHANGELOG.md](CHANGELOG.md) for release notes. Licensed under the terms in
[LICENSE](LICENSE).
