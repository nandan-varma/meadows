# Meadows Language Server & VS Code Extension

Two TypeScript packages, independent of the C++ compiler's build system:

- **`meadows-lsp/`** — an LSP server (`vscode-languageserver`). It spawns the
  `Meadows` CLI with `--lsp-diagnostics <file>` per document change, parses
  the JSON diagnostics it prints on stdout (see `src/lsp/LSPInterface.cpp`
  in the compiler), and forwards them to the editor. Also provides hover
  and semantic-token support (`src/hover-provider.ts`,
  `src/semantic-tokens.ts`).
- **`meadows-vscode/`** — a VS Code extension bundling the server above plus
  a TextMate grammar (`syntaxes/meadows.tmLanguage.json`) for syntax
  highlighting.

The extension loads the server via a relative path
(`../meadows-lsp/dist/server.js`), so the two packages must stay siblings on
disk, in this layout, and `meadows-lsp` must be built before the extension
can find it.

## Prerequisites

- Node.js 18+
- The `Meadows` compiler binary on your `PATH` (build it first — see the
  [root README](../README.md#quick-start)). The server spawns it by name
  (`Meadows`, case-sensitive on Linux); there's currently no setting to
  point it at a different location.

## Build

```bash
cd lsp/meadows-lsp
npm install
npm run build          # compiles src/ -> dist/server.js

cd ../meadows-vscode
npm install
npm run build           # compiles src/ -> dist/extension.js
```

## Run the extension

Open `lsp/meadows-vscode/` in VS Code and press F5 (Run Extension) to launch
an Extension Development Host with it loaded. To package a `.vsix` for
manual installation, use [`vsce`](https://github.com/microsoft/vscode-vsce)
(`npx vsce package` from `lsp/meadows-vscode/`).

## Iterating on the server

```bash
cd lsp/meadows-lsp
npm run watch           # recompiles on change
```

Reload the Extension Development Host window to pick up changes.
