import { build } from "esbuild";
import { createHash } from "node:crypto";
import { existsSync } from "node:fs";
import { copyFile, mkdir, readFile, readdir, rm, writeFile } from "node:fs/promises";
import { dirname, join, relative, resolve } from "node:path";
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

const shared = {
  outdir: output,
  bundle: true,
  target: "es2022",
  minify: true,
  logLevel: "info",
};

await build({
  ...shared,
  entryPoints: { main: join(root, "src/main.js"), worker: join(root, "src/worker.js") },
  format: "esm",
  splitting: true,
  sourcemap: true,
  external: ["./meadows.js"],
  loader: { ".woff2": "file" },
  assetNames: "assets/[name]-[hash]",
});

await build({
  ...shared,
  entryPoints: { boot: join(root, "src/boot.js") },
  format: "iife",
});

for (const file of ["index.html", "favicon.svg", "manifest.webmanifest"]) {
  await copyFile(join(root, file), join(output, file));
}
for (const artifact of ["meadows.js", "meadows.wasm"]) {
  await copyFile(join(wasmDirectory, artifact), join(output, artifact));
}

async function listFiles(directory) {
  const entries = await readdir(directory, { withFileTypes: true });
  const nested = await Promise.all(
    entries.map((entry) =>
      entry.isDirectory() ? listFiles(join(directory, entry.name)) : [join(directory, entry.name)],
    ),
  );
  return nested.flat();
}

const assets = (await listFiles(output))
  .map((file) => relative(output, file))
  .filter((file) => !file.endsWith(".map"))
  .sort();
const hash = createHash("sha256");
for (const asset of assets) {
  hash.update(asset).update(await readFile(join(output, asset)));
}
const worker = (await readFile(join(root, "src/sw.js"), "utf8"))
  .replace("__VERSION__", hash.digest("hex").slice(0, 12))
  .replace("__ASSETS__", JSON.stringify(["./", ...assets]));
await writeFile(join(output, "sw.js"), worker);
