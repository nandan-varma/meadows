import assert from "node:assert/strict";
import { test } from "node:test";

import { formatSource } from "../src/format.js";
import { parseBytecode, parseTokens, parseTree, splitLocations } from "../src/parse.js";
import { DEFAULTS, normaliseSettings } from "../src/settings.js";
import { findLocals, findSymbols } from "../src/symbols.js";
import { createZip, crc32 } from "../src/zip.js";
import { ENTRY_FILE, createProject, renameFile } from "../src/project.js";

test("formatting re-indents blocks, arrays and else chains", () => {
  const messy = 'func f(a){\nif (a){\nprint("}");\n}else{\nreturn [\n1,\n2\n];\n}\n}   \n\n\n\nf(1);';
  assert.equal(
    formatSource(messy),
    'func f(a){\n  if (a){\n    print("}");\n  }else{\n    return [\n      1,\n      2\n    ];\n  }\n}\n\nf(1);\n',
  );
  assert.equal(formatSource("\n\n"), "");
  assert.equal(formatSource(formatSource(messy)), formatSource(messy));
});

test("symbols list functions and top-level variables with positions", () => {
  const symbols = findSymbols([
    { name: "main.ms", text: "# func hidden() {}\nlet total = 0;\n  let inner = 1;\nfunc add(a, b) {\n}\n" },
  ]);
  assert.deepEqual(
    symbols.map(({ kind, name, signature, line, column }) => [kind, name, signature, line, column]),
    [
      ["variable", "total", "total", 2, 5],
      ["function", "add", "add(a, b)", 4, 6],
    ],
  );
});

test("locals include parameters, lets and loop variables but not comments", () => {
  const names = findLocals('func f(a, b) {\n  let c = 1; // let hidden = 2\n  for (i in range(0, a)) {}\n}\n');
  assert.deepEqual(names.sort(), ["a", "b", "c", "i"]);
});

test("token, tree and location parsing", () => {
  assert.deepEqual(parseTokens("1:1  LET  'let'\n2:5  EOF\n"), [
    { line: 1, column: 1, type: "LET", text: "let" },
    { line: 2, column: 5, type: "EOF", text: "" },
  ]);
  const tree = parseTree("Func f\n  Return\n    Int 1\nExprStmt\n");
  assert.equal(tree.length, 2);
  assert.equal(tree[0].children[0].children[0].label, "Int 1");
  assert.deepEqual(splitLocations("at main.ms:3:9 and other.ms:1:1", new Set(["main.ms"])), [
    { text: "at " },
    { text: "main.ms:3:9", file: "main.ms", line: 3, column: 9 },
    { text: " and other.ms:1:1" },
  ]);
});

test("bytecode listings are split into functions and rows", () => {
  const listing = "func f  params 1  slots 1\n    0  Load         slot 0\n    1  Div          ; line 2\n\nfunc main  params 0  slots 0\n    0  Halt\n";
  const functions = parseBytecode(listing);
  assert.deepEqual(functions.map((f) => [f.name, f.params, f.rows.length]), [["f", 1, 2], ["main", 0, 1]]);
  assert.deepEqual(functions[0].rows[1], { index: 1, op: "Div", operands: "", line: 2 });
});

test("settings fall back to defaults for invalid values", () => {
  assert.deepEqual(normaliseSettings(null), DEFAULTS);
  const settings = normaliseSettings({ theme: "neon", fontSize: 16, vim: "yes", split: 0.9 });
  assert.equal(settings.theme, DEFAULTS.theme);
  assert.equal(settings.fontSize, 16);
  assert.equal(settings.vim, false);
  assert.equal(settings.split, DEFAULTS.split);
});

test("zip archives list every file with its checksum", async () => {
  assert.equal(crc32(new TextEncoder().encode("123456789")), 0xcbf43926);
  const blob = createZip([{ name: "main.ms", text: "print(1);\n" }, { name: "lib.ms", text: "" }]);
  const bytes = new Uint8Array(await blob.arrayBuffer());
  const view = new DataView(bytes.buffer);
  assert.equal(view.getUint32(0, true), 0x04034b50);
  assert.equal(view.getUint16(bytes.length - 12, true), 2);
  const text = new TextDecoder().decode(bytes);
  assert.ok(text.includes("main.ms") && text.includes("lib.ms") && text.includes("print(1);"));
});

test("files can be renamed except the entry point", () => {
  const project = createProject([{ name: ENTRY_FILE, text: "" }, { name: "a.ms", text: "" }]);
  project.active = "a.ms";
  renameFile(project, "a.ms", "b.ms");
  assert.equal(project.active, "b.ms");
  assert.throws(() => renameFile(project, ENTRY_FILE, "x.ms"));
  assert.throws(() => renameFile(project, "b.ms", ENTRY_FILE));
  assert.throws(() => renameFile(project, "b.ms", "bad name.ms"));
});
