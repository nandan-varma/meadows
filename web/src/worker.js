import createMeadows from "./meadows.js";

const PROJECT_DIRECTORY = "/project";
const FLUSH_INTERVAL_MS = 30;

const ready = createMeadows().then((module) => {
  module.FS.mkdirTree(PROJECT_DIRECTORY);
  module.FS.chdir(PROJECT_DIRECTORY);
  return module;
});

function installFiles(module, files) {
  const { FS } = module;
  const wanted = new Set(files.map((file) => file.name));
  for (const existing of FS.readdir(PROJECT_DIRECTORY)) {
    if (existing.endsWith(".ms") && !wanted.has(existing)) {
      FS.unlink(existing);
    }
  }
  for (const file of files) {
    FS.writeFile(file.name, file.text);
  }
}

function analyze(module, { files, entry }) {
  installFiles(module, files);
  const report = module.analyze(entry);
  return {
    ok: report.ok,
    diagnostics: report.diagnostics,
    diagnosticsJson: report.diagnosticsJson,
    tokens: report.tokens,
    ast: report.ast,
    bytecode: report.bytecode,
  };
}

function execute(module, { files, entry }) {
  installFiles(module, files);
  const decoder = new TextDecoder();
  let pending = "";
  let lastFlush = performance.now();

  const flush = () => {
    if (pending) {
      self.postMessage({ type: "output", text: pending });
      pending = "";
    }
    lastFlush = performance.now();
  };

  const started = performance.now();
  const report = module.execute(entry, (bytes) => {
    pending += decoder.decode(bytes, { stream: true });
    if (performance.now() - lastFlush >= FLUSH_INTERVAL_MS) {
      flush();
    }
  });
  pending += decoder.decode();
  flush();

  return {
    compiled: report.compiled,
    exitCode: report.exitCode,
    diagnostics: report.diagnostics,
    diagnosticsJson: report.diagnosticsJson,
    runtimeError: report.runtimeError,
    elapsed: performance.now() - started,
  };
}

self.onmessage = async ({ data }) => {
  const module = await ready;
  try {
    if (data.type === "analyze") {
      self.postMessage({ type: "analysis", id: data.id, report: analyze(module, data) });
    } else if (data.type === "run") {
      self.postMessage({ type: "done", report: execute(module, data) });
    }
  } catch (error) {
    self.postMessage({ type: "failure", id: data.id, message: String(error) });
  }
};

self.postMessage({ type: "ready", version: (await ready).version() });
