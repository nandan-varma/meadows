# Playground

The playground is a static site in `web/`. It runs the interpreter as
WebAssembly inside Web Workers.

## Build

```bash
emcmake cmake -B build-wasm -DBUILD_WASM=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
cd web
npm ci
npm run build      # writes web/dist
npm run serve      # http://localhost:8080 (PORT overrides)
```

`npm test` checks the project model, share links, formatter, symbol finder,
parsers, settings and zip writer, and compiles every example when
`MEADOWS_WASM` points at `meadows.js`.

## Features

- **Command palette** (`Ctrl/⌘ K`) reaches every command, file, symbol and
  example. `Ctrl/⌘ Shift O` jumps to a symbol.
- **Editor**: syntax highlighting, inline diagnostics, completion with
  documentation and snippets, hover documentation, go to definition
  (`F12` or `Ctrl/⌘`-click), folding, find, format (`Shift Alt F`, also on
  `Ctrl/⌘ S`), optional Vim keys.
- **Results**: Output with clickable error locations, Problems, Tokens, an
  expandable Syntax tree, the interpreter Bytecode, and a searchable Language
  reference whose examples insert at the cursor.
- **Run on change** reruns the program when it has no errors; **Benchmark**
  runs it twenty times and reports fastest, median, mean and slowest.
- **Projects**: `main.ms` plus up to fifteen files in an in-memory file system,
  so `import "name.ms";` works as on the command line. Add, rename and delete
  files, drop or import `.ms` files, download a file or the project as `.zip`.
- **Share** gives a link and an iframe snippet (`?embed` hides the chrome).
  The project lives in the link's fragment; nothing is sent to a server.
- Themes (system, light, dark, midnight), font size, word wrap, stacked or side
  by side layout and a draggable divider. Settings and the project are kept in
  local storage.
- Installable and available offline through a service worker.

## Behavior

- Run starts a fresh worker. Output streams as the program prints; Stop
  terminates the worker, and output beyond 1,000,000 characters stops the run.
- Analysis runs in a second worker as you type.
- The bytecode view lists exactly what the interpreter executes; native builds
  generate LLVM IR instead, which is only available from the command line with
  `--dump-ir`.

## Deployment

`.github/workflows/deploy-playground.yml` builds the module, runs the tests and
publishes `web/dist` to GitHub Pages on every push to `main` that touches the
compiler or the site.
