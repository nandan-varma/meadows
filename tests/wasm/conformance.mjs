// Runs the conformance suite against the WebAssembly interpreter.
// Usage: node conformance.mjs <path to meadows.js> [--update]

import { readdirSync, readFileSync, statSync, existsSync } from "node:fs";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const suite = resolve(here, "../conformance");
const modulePath = resolve(process.argv[2]);
const { default: createMeadows } = await import(pathToFileURL(modulePath));

function walk(directory) {
  return readdirSync(directory).flatMap((name) => {
    const path = join(directory, name);
    return statSync(path).isDirectory() ? walk(path) : [path];
  });
}

const sources = walk(suite)
  .filter((path) => path.endsWith(".ms"))
  .map((path) => relative(suite, path));

async function instantiate() {
  const module = await createMeadows();
  const { FS } = module;
  for (const path of sources) {
    FS.mkdirTree(join("/suite", dirname(path)));
    FS.writeFile(join("/suite", path), readFileSync(join(suite, path)));
  }
  FS.chdir("/suite");
  return module;
}

function expectation(path, extension) {
  const file = join(suite, path.replace(/\.ms$/, extension));
  return existsSync(file) ? readFileSync(file, "utf8") : "";
}

let passed = 0;
let failed = 0;

function check(path, label, actual, expected) {
  if (actual === expected) return true;
  failed += 1;
  console.log(`FAIL ${path}: ${label} differs`);
  console.log(`  expected: ${JSON.stringify(expected)}`);
  console.log(`  actual:   ${JSON.stringify(actual)}`);
  return false;
}

for (const path of sources) {
  if (path.startsWith("modules/")) continue;
  const module = await instantiate();

  if (path.startsWith("diagnostics/")) {
    const report = module.analyze(path);
    if (check(path, "diagnostics", report.diagnostics, expectation(path, ".expected_error"))) {
      passed += 1;
    }
    continue;
  }

  const decoder = new TextDecoder();
  let output = "";
  const report = module.execute(path, (bytes) => {
    output += decoder.decode(bytes, { stream: true });
  });
  output += decoder.decode();

  const expectedError = expectation(path, ".expected_error");
  const ok =
    check(path, "compilation", report.compiled, true) &&
    check(path, "stdout", output, expectation(path, ".expected")) &&
    check(path, "stderr", report.runtimeError, expectedError) &&
    check(path, "exit code", report.exitCode, expectedError ? 1 : 0);
  if (ok) passed += 1;
}

console.log(`wasm: ${passed} passed, ${failed} failed`);
process.exit(failed === 0 ? 0 : 1);
