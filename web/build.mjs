import { build } from "esbuild";
import { copyFile, mkdir, rm } from "node:fs/promises";
import { existsSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = dirname(fileURLToPath(import.meta.url));
const output = join(root, "dist");
const wasmDirectory = resolve(root, process.argv[2] ?? "../build-wasm/bin");

for (const artifact of ["meadows.js", "meadows.wasm"]) {
  if (!existsSync(join(wasmDirectory, artifact))) {
    console.error(`Missing ${artifact} in ${wasmDirectory}; build the WebAssembly module first.`);
    process.exit(1);
  }
}

await rm(output, { recursive: true, force: true });
await mkdir(output, { recursive: true });

await build({
  entryPoints: { main: join(root, "src/main.js"), worker: join(root, "src/worker.js") },
  outdir: output,
  bundle: true,
  format: "esm",
  target: "es2022",
  minify: true,
  sourcemap: true,
  external: ["./meadows.js"],
  logLevel: "info",
});

for (const file of ["index.html", "styles.css", "favicon.svg"]) {
  await copyFile(join(root, file), join(output, file));
}
for (const artifact of ["meadows.js", "meadows.wasm"]) {
  await copyFile(join(wasmDirectory, artifact), join(output, artifact));
}
