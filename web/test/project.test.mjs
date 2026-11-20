import assert from "node:assert/strict";
import { test } from "node:test";

import {
  ENTRY_FILE,
  addFile,
  createProject,
  decodeProject,
  encodeProject,
  isValidFileName,
  removeFile,
} from "../src/project.js";
import { EXAMPLES } from "../src/examples.js";

const sample = () =>
  createProject([
    { name: ENTRY_FILE, text: 'import "lib.ms";\nprint("héllo ✓");\n' },
    { name: "lib.ms", text: "func f() { return 1; }\n" },
  ]);

test("file names are validated", () => {
  assert.ok(isValidFileName("a_b-1.ms"));
  for (const bad of ["a.txt", "../a.ms", "a/b.ms", ".ms", "a b.ms", ""]) {
    assert.ok(!isValidFileName(bad), bad);
  }
});

test("files can be added and removed, but not the entry point", () => {
  const project = sample();
  addFile(project, "util.ms");
  assert.equal(project.active, "util.ms");
  assert.throws(() => addFile(project, "util.ms"), /already exists/);
  assert.throws(() => addFile(project, "bad.txt"), /must end in .ms/);
  removeFile(project, "util.ms");
  assert.equal(project.active, ENTRY_FILE);
  assert.throws(() => removeFile(project, ENTRY_FILE), /entry point/);
});

test("projects survive a share-link round trip", async () => {
  const project = sample();
  const decoded = await decodeProject(await encodeProject(project));
  assert.deepEqual(decoded, project);
});

test("malformed share links are rejected", async () => {
  for (const text of ["", "###", "AAAA", await encodeProject({ files: [] })]) {
    assert.equal(await decodeProject(text), null);
  }
});

test("every example has an entry point and valid file names", () => {
  for (const example of EXAMPLES) {
    assert.ok(example.files.some((file) => file.name === ENTRY_FILE), example.id);
    for (const file of example.files) {
      assert.ok(isValidFileName(file.name), `${example.id}/${file.name}`);
    }
  }
});
