import assert from "node:assert/strict";
import { existsSync } from "node:fs";
import { resolve } from "node:path";
import { test } from "node:test";
import { pathToFileURL } from "node:url";

import { EXAMPLES } from "../src/examples.js";

const wasm = resolve(process.env.MEADOWS_WASM ?? "../build-wasm/bin/meadows.js");

test("every example compiles", { skip: !existsSync(wasm) && "WebAssembly module not built" }, async () => {
  const { default: create } = await import(pathToFileURL(wasm));
  for (const example of EXAMPLES) {
    const module = await create();
    for (const file of example.files) {
      module.FS.writeFile(file.name, file.text);
    }
    const report = module.analyze("main.ms");
    assert.ok(report.ok, `${example.id}: ${report.diagnostics}`);
    assert.equal(report.diagnostics, "", `${example.id} has warnings`);
  }
});
