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

`npm test` checks the project model and share links, and compiles every example
when `MEADOWS_WASM` points at `meadows.js`.

## Behavior

- Files `main.ms` plus up to fifteen others live in an in-memory file system, so
  `import "name.ms";` works as it does on the command line.
- Run starts a fresh worker. Output streams as the program prints; Stop
  terminates the worker, and output beyond 1,000,000 characters stops the run.
- Analysis runs in a second worker as you type and marks diagnostics in the
  editor. The Problems, Tokens and Syntax tree tabs show the same analysis.
- Share copies a link whose fragment holds the compressed project; nothing is
  sent to a server. The project is also kept in local storage.

## Deployment

`.github/workflows/deploy-playground.yml` builds the module, runs the tests and
publishes `web/dist` to GitHub Pages on every push to `main` that touches the
compiler or the site.
