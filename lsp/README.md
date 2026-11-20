# Meadows editor support

Two TypeScript packages that sit beside the compiler:

- **`meadows-lsp/`**: a language server (`vscode-languageserver`). On each
  change it runs `Meadows --lsp-diagnostics --stdin <path>`, sending the unsaved
  buffer and naming the document's real path so relative imports resolve, and
  forwards the JSON diagnostics for that file. It also provides hover and
  semantic tokens.
- **`meadows-vscode/`**: a VS Code extension that starts the server and adds a
  TextMate grammar and language configuration.

The extension loads the server from `../meadows-lsp/dist/server.js`, so the two
packages must stay siblings and `meadows-lsp` must be built first.

## Build

Requires Node.js 18 or later and the `Meadows` compiler on your `PATH` (or set
`meadows.compilerPath` in VS Code).

```bash
cd lsp/meadows-lsp && npm ci && npm run build
cd ../meadows-vscode && npm ci && npm run build
```

## Run

Open `lsp/meadows-vscode` in VS Code and press F5 to launch an Extension
Development Host, then open any `.ms` file.
