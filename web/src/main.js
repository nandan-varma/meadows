import "../styles.css";
import { ask, confirmAction, openModal } from "./dialogs.js";
import { el, icon, isMac, kbd } from "./dom.js";
import { Editor } from "./editor.js";
import { EXAMPLES, findExample } from "./examples.js";
import { openPalette } from "./palette.js";
import {
  ENTRY_FILE,
  activeFile,
  addFile,
  createProject,
  decodeProject,
  encodeProject,
  isValidFileName,
  loadSavedProject,
  removeFile,
  renameFile,
  saveProject,
  upsertFile,
} from "./project.js";
import { Analyzer, Runner } from "./runner.js";
import { Settings, THEMES } from "./settings.js";
import { findSymbols } from "./symbols.js";
import { toast } from "./toast.js";
import { Views } from "./views.js";
import { createZip } from "./zip.js";

const ANALYSIS_DELAY_MS = 300;
const AUTORUN_DELAY_MS = 600;
const BENCHMARK_RUNS = 20;
const SHARE_PREFIX = "#project=";
const DEFAULT_EXAMPLE = "fibonacci";
const NARROW = window.matchMedia("(max-width: 820px)");
const DARK = window.matchMedia("(prefers-color-scheme: dark)");

const $ = (id) => document.getElementById(id);
const dom = {
  workspace: $("workspace"),
  divider: $("divider"),
  files: $("files"),
  addFile: $("add-file"),
  run: $("run"),
  stop: $("stop"),
  share: $("share"),
  theme: $("theme"),
  layout: $("layout"),
  views: $("views"),
  problemCount: $("problem-count"),
  status: $("status"),
  cursor: $("cursor"),
  mode: $("mode"),
  version: $("version"),
  drop: $("drop"),
};

const settings = new Settings();
const runner = new Runner();
const analyzer = new Analyzer();
const views = new Views($("results"), {
  fileNames: () => project.files.map((file) => file.name),
  onNavigate: navigate,
  onInsert: (text) => editor.insert(text),
});
const editor = new Editor($("editor"), {
  onChange: handleEdit,
  onRun: run,
  onCursor: (line, column, selected) => {
    dom.cursor.textContent = `Ln ${line}, Col ${column}${selected ? ` (${selected} selected)` : ""}`;
  },
  symbols: () => findSymbols(project.files),
  onDefinition: goToDefinition,
});

let project;
let edited = false;
let diagnostics = [];
let analysisTimer;
let autorunTimer;
let autorunPending = false;
let benchmarking = false;
let cancelled = false;

const modifier = isMac() ? "⌘" : "Ctrl";

function setStatus(text) {
  dom.status.textContent = text;
}

function plural(count, word) {
  return `${count} ${word}${count === 1 ? "" : "s"}`;
}

function describeProblems() {
  const errors = diagnostics.filter((d) => d.severity === 1).length;
  if (diagnostics.length === 0) {
    return "No problems";
  }
  return `${plural(errors, "error")}, ${plural(diagnostics.length - errors, "warning")}`;
}

function formatDuration(milliseconds) {
  return milliseconds < 1
    ? `${(milliseconds * 1000).toFixed(0)} µs`
    : milliseconds < 1000
      ? `${milliseconds.toFixed(milliseconds < 10 ? 1 : 0)} ms`
      : `${(milliseconds / 1000).toFixed(2)} s`;
}

function resolvedTheme() {
  const theme = settings.get("theme");
  return theme === "system" ? (DARK.matches ? "dark" : "light") : theme;
}

function applySettings(key = null) {
  const theme = settings.get("theme");
  if (theme === "system") {
    delete document.documentElement.dataset.theme;
  } else {
    document.documentElement.dataset.theme = theme;
  }
  document.documentElement.style.setProperty("--editor-font-size", `${settings.get("fontSize")}px`);
  const stacked = settings.get("stacked") || NARROW.matches;
  dom.workspace.classList.toggle("stacked", stacked);
  dom.workspace.style.setProperty("--split", `${(settings.get("split") * 100).toFixed(1)}%`);
  dom.divider.setAttribute("aria-orientation", stacked ? "horizontal" : "vertical");
  dom.layout.replaceChildren(icon(settings.get("stacked") ? "columns" : "rows"));
  dom.theme.replaceChildren(icon(resolvedTheme() === "light" ? "sun" : "moon"));
  dom.theme.title = `Theme: ${theme}`;
  const modes = [settings.get("vim") ? "VIM" : null, settings.get("autorun") ? "AUTO-RUN" : null].filter(Boolean);
  dom.mode.hidden = modes.length === 0;
  dom.mode.textContent = modes.join(" · ");
  if (key === null || key === "vim" || key === "wrap") {
    editor.configure({ vim: settings.get("vim"), wrap: settings.get("wrap") });
  }
}

function renderFiles() {
  dom.files.replaceChildren(
    ...project.files.map((file) => {
      const tab = el(
        "button",
        {
          type: "button",
          class: "file-tab",
          role: "tab",
          "aria-selected": String(file.name === project.active),
          tabindex: file.name === project.active ? "0" : "-1",
          onclick: () => openFile(file.name),
          ondblclick: () => file.name !== ENTRY_FILE && renameActive(file.name),
        },
        file.name,
      );
      const close =
        file.name === ENTRY_FILE
          ? null
          : el(
              "button",
              {
                type: "button",
                class: "file-close",
                title: `Delete ${file.name}`,
                "aria-label": `Delete ${file.name}`,
                onclick: () => deleteFile(file.name),
              },
              icon("x"),
            );
      return el("span", { class: "file-tab-group" }, tab, close);
    }),
  );
}

function openFile(name) {
  project.active = name;
  editor.open(name, activeFile(project).text);
  renderFiles();
  showDiagnosticsForActiveFile();
  saveProject(project);
  editor.focus();
}

function navigate(file, line, column) {
  if (!project.files.some((candidate) => candidate.name === file)) {
    return;
  }
  if (project.active !== file) {
    openFile(file);
  }
  editor.goTo(line, column);
}

function goToDefinition(name) {
  const symbol = findSymbols(project.files).find((candidate) => candidate.name === name);
  if (!symbol) {
    return false;
  }
  navigate(symbol.file, symbol.line, symbol.column);
  return true;
}

async function deleteFile(name) {
  const confirmed = await confirmAction({
    title: `Delete ${name}?`,
    message: "This removes the file from the project.",
    confirm: "Delete",
  });
  if (!confirmed) {
    return;
  }
  removeFile(project, name);
  editor.forget(name);
  openFile(project.active);
  scheduleAnalysis(0);
}

async function renameActive(name = project.active) {
  if (name === ENTRY_FILE) {
    toast(`${ENTRY_FILE} is the entry point and cannot be renamed`, "error");
    return;
  }
  const next = await ask({ title: "Rename file", label: "File name", value: name, confirm: "Rename" });
  if (!next || next === name) {
    return;
  }
  try {
    renameFile(project, name, next);
  } catch (error) {
    toast(error.message, "error");
    return;
  }
  editor.rename(name, next);
  renderFiles();
  saveProject(project);
  scheduleAnalysis(0);
}

async function newFile() {
  const answer = await ask({ title: "New file", label: "File name", value: "", confirm: "Create" });
  if (!answer) {
    return;
  }
  const name = answer.includes(".") ? answer : `${answer}.ms`;
  try {
    addFile(project, name);
  } catch (error) {
    toast(error.message, "error");
    return;
  }
  renderFiles();
  openFile(name);
  scheduleAnalysis(0);
}

function loadProject(next) {
  project = next;
  edited = false;
  diagnostics = [];
  editor.reset();
  views.clearOutput();
  openFile(project.active);
  scheduleAnalysis(0);
}

function handleEdit(name, text) {
  const file = project.files.find((candidate) => candidate.name === name);
  if (!file) {
    return;
  }
  file.text = text;
  edited = true;
  saveProject(project);
  scheduleAnalysis(ANALYSIS_DELAY_MS);
  autorunPending = settings.get("autorun");
}

function showDiagnosticsForActiveFile() {
  editor.showDiagnostics(diagnostics.filter((d) => d.file === project.active));
}

function scheduleAnalysis(delay) {
  clearTimeout(analysisTimer);
  analysisTimer = setTimeout(refreshAnalysis, delay);
}

async function refreshAnalysis() {
  let report;
  try {
    report = await analyzer.analyze(project, ENTRY_FILE);
  } catch (error) {
    setStatus(`Analysis failed: ${error.message}`);
    return;
  }
  if (!report) {
    return;
  }
  diagnostics = JSON.parse(report.diagnosticsJson);
  views.renderAnalysis(report, diagnostics);

  const errors = diagnostics.filter((d) => d.severity === 1).length;
  dom.problemCount.hidden = diagnostics.length === 0;
  dom.problemCount.textContent = String(diagnostics.length);
  dom.problemCount.classList.toggle("warnings-only", errors === 0);

  showDiagnosticsForActiveFile();
  if (!runner.running) {
    setStatus(describeProblems());
  }
  if (autorunPending && errors === 0) {
    autorunPending = false;
    clearTimeout(autorunTimer);
    autorunTimer = setTimeout(() => {
      runner.stop();
      run({ quiet: true });
    }, AUTORUN_DELAY_MS);
  }
}

function setRunning(running) {
  dom.run.disabled = running;
  dom.run.classList.toggle("running", running);
  dom.stop.disabled = !running;
}

function lineCount(text) {
  return (text.match(/\n/g) ?? []).length;
}

function finishRun(result) {
  setRunning(false);
  const took = formatDuration(result.elapsed);
  const lines = plural(lineCount(views.outputText), "line");

  if (result.status === "stopped") {
    views.appendNote("\nStopped.\n");
    views.setMeta([{ label: "Stopped" }, { label: took }]);
    setStatus(`Stopped after ${took}`);
  } else if (result.status === "limit") {
    views.appendError("\nOutput limit reached; execution stopped.\n");
    views.setMeta([{ label: "Output limit reached", kind: "fail" }]);
    setStatus("Stopped: output limit reached");
  } else if (result.status === "failure") {
    views.appendError(`\n${result.message}\n`);
    views.setMeta([{ label: "Could not run", kind: "fail" }]);
    setStatus("The program could not be run");
  } else if (!result.report.compiled) {
    views.appendError(result.report.diagnostics);
    views.appendNote("\nFix the problems above to run the program.\n");
    views.setMeta([{ label: describeProblems(), kind: "fail" }]);
    setStatus(describeProblems());
  } else if (result.report.exitCode !== 0) {
    const gap = views.outputText === "" || views.outputText.endsWith("\n") ? "" : "\n";
    views.appendError(gap + result.report.runtimeError);
    views.setMeta([{ label: "Failed", kind: "fail" }, { label: took }, { label: lines }]);
    setStatus(`Failed after ${took}`);
  } else {
    views.setMeta([{ label: "Finished", kind: "ok" }, { label: took }, { label: lines }]);
    setStatus(`Finished in ${took}`);
  }
}

function run({ quiet = false } = {}) {
  if (runner.running || benchmarking) {
    return;
  }
  views.clearOutput();
  if (!quiet || views.current !== "output") {
    views.show("output");
  }
  setRunning(true);
  setStatus("Running…");
  views.setMeta([{ label: "Running…" }]);
  runner.run(project, ENTRY_FILE, {
    onOutput: (text) => views.appendOutput(text),
    onFinish: finishRun,
  });
}

function runSilently() {
  return new Promise((resolve) => {
    runner.run(project, ENTRY_FILE, { onOutput: () => {}, onFinish: resolve });
  });
}

function median(sorted) {
  const middle = Math.floor(sorted.length / 2);
  return sorted.length % 2 ? sorted[middle] : (sorted[middle - 1] + sorted[middle]) / 2;
}

async function benchmark() {
  if (runner.running || benchmarking) {
    return;
  }
  if (diagnostics.some((d) => d.severity === 1)) {
    toast("Fix the errors before benchmarking", "error");
    return;
  }
  benchmarking = true;
  cancelled = false;
  views.clearOutput();
  views.show("output");
  setRunning(true);
  const times = [];
  let interrupted = false;
  for (let i = 0; i < BENCHMARK_RUNS; i++) {
    setStatus(`Benchmarking… ${i + 1}/${BENCHMARK_RUNS}`);
    views.setMeta([{ label: `Benchmarking ${i + 1}/${BENCHMARK_RUNS}` }]);
    const result = await runSilently();
    if (cancelled || result.status !== "done" || !result.report.compiled || result.report.exitCode !== 0) {
      interrupted = true;
      break;
    }
    times.push(result.elapsed);
  }
  benchmarking = false;
  setRunning(false);
  if (interrupted || times.length === 0) {
    views.appendNote(
      cancelled
        ? "Benchmark stopped.\n"
        : "The program must finish without errors to be benchmarked.\n",
    );
    views.setMeta([{ label: "Benchmark stopped" }]);
    setStatus("Benchmark stopped");
    return;
  }
  const sorted = [...times].sort((a, b) => a - b);
  const mean = times.reduce((sum, value) => sum + value, 0) / times.length;
  const rows = [
    ["runs", String(times.length)],
    ["fastest", formatDuration(sorted[0])],
    ["median", formatDuration(median(sorted))],
    ["mean", formatDuration(mean)],
    ["slowest", formatDuration(sorted.at(-1))],
  ];
  views.appendOutput(rows.map(([label, value]) => `${label.padEnd(8)}${value}`).join("\n") + "\n");
  views.setMeta([{ label: "Benchmark", kind: "ok" }, { label: `median ${formatDuration(median(sorted))}` }]);
  setStatus(`Median ${formatDuration(median(sorted))} over ${times.length} runs`);
}

function stop() {
  cancelled = benchmarking;
  runner.stop();
}

async function copyText(text, message) {
  try {
    await navigator.clipboard.writeText(text);
    toast(message);
    return true;
  } catch {
    toast("Copying is not available here; select the text instead", "error");
    return false;
  }
}

async function share() {
  const base = `${location.origin}${location.pathname}`;
  const encoded = await encodeProject(project);
  const link = `${base}${SHARE_PREFIX}${encoded}`;
  const embed = `<iframe src="${base}?embed${SHARE_PREFIX}${encoded}" width="100%" height="480" style="border:1px solid #ccc;border-radius:8px" title="Meadows playground"></iframe>`;

  const row = (label, value) => {
    const input = el("input", { type: "text", value, readonly: true, "aria-label": label });
    input.addEventListener("focus", () => input.select());
    return el(
      "label",
      {},
      label,
      el(
        "div",
        { class: "copy-row" },
        input,
        el("button", { type: "button", onclick: () => copyText(value, `${label} copied`) }, icon("copy"), "Copy"),
      ),
    );
  };
  openModal(
    "wide",
    el(
      "div",
      { class: "sheet" },
      el("h2", {}, "Share this project"),
      el("p", {}, `The whole project is stored in the link (${link.length.toLocaleString()} characters). Nothing is uploaded.`),
      row("Link", link),
      row("Embed code", embed),
    ),
  );
}

function download(blob, name) {
  const url = URL.createObjectURL(blob);
  const anchor = el("a", { href: url, download: name });
  document.body.append(anchor);
  anchor.click();
  anchor.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}

function exportZip() {
  download(createZip(project.files), "meadows-project.zip");
}

function exportFile() {
  const file = activeFile(project);
  download(new Blob([file.text], { type: "text/plain" }), file.name);
}

async function importFiles(fileList) {
  let imported = 0;
  for (const file of fileList) {
    if (!isValidFileName(file.name)) {
      toast(`${file.name} is not a valid Meadows file name`, "error");
      continue;
    }
    try {
      upsertFile(project, file.name, await file.text());
      editor.forget(file.name);
      imported++;
    } catch (error) {
      toast(error.message, "error");
      break;
    }
  }
  if (imported > 0) {
    toast(`Added ${plural(imported, "file")}`);
    renderFiles();
    openFile(project.active);
    saveProject(project);
    scheduleAnalysis(0);
  }
}

function pickFiles() {
  const input = el("input", { type: "file", accept: ".ms", multiple: true });
  input.addEventListener("change", () => importFiles([...input.files]));
  input.click();
}

async function chooseExample(example) {
  if (edited) {
    const confirmed = await confirmAction({
      title: `Open “${example.title}”?`,
      message: "This replaces the files in your project.",
      confirm: "Replace project",
    });
    if (!confirmed) {
      return;
    }
  }
  loadProject(createProject(example.files));
  history.replaceState(null, "", location.pathname + location.search);
}

function cycleTheme() {
  const next = THEMES[(THEMES.indexOf(settings.get("theme")) + 1) % THEMES.length];
  settings.set("theme", next);
  toast(`Theme: ${next}`);
}

function changeFontSize(delta) {
  settings.set("fontSize", Math.min(24, Math.max(11, settings.get("fontSize") + delta)));
}

function toggle(key, label) {
  settings.set(key, !settings.get(key));
  toast(`${label} ${settings.get(key) ? "on" : "off"}`);
}

function showShortcuts() {
  const rows = [
    ["Run the program", `Mod+Enter`],
    ["Command palette", "Mod+K"],
    ["Go to symbol", "Mod+Shift+O"],
    ["Format the document", "Shift+Alt+F"],
    ["Find in file", "Mod+F"],
    ["Go to definition", "F12"],
    ["Fold or unfold a block", "Ctrl+Shift+["],
    ["Trigger completion", "Ctrl+Space"],
  ];
  openModal(
    "wide",
    el(
      "div",
      { class: "sheet" },
      el("h2", {}, "Keyboard shortcuts"),
      el("dl", { class: "shortcuts" }, ...rows.flatMap(([label, keys]) => [el("dt", {}, label), el("dd", {}, kbd(keys))])),
      el("p", {}, `Hold ${modifier} and click an identifier to jump to its definition. Hover for documentation.`),
    ),
  );
}

function symbolItems() {
  return findSymbols(project.files).map((symbol) => ({
    title: symbol.signature,
    detail: `${symbol.file}:${symbol.line}`,
    group: symbol.kind === "function" ? "Function" : "Variable",
    run: () => navigate(symbol.file, symbol.line, symbol.column),
  }));
}

function viewCommands() {
  return [
    ["output", "Output"],
    ["problems", "Problems"],
    ["tokens", "Tokens"],
    ["ast", "Syntax tree"],
    ["bytecode", "Bytecode"],
    ["reference", "Language reference"],
  ].map(([view, title]) => ({ title: `Show ${title}`, group: "View", run: () => views.show(view) }));
}

function commandItems() {
  const flag = (key) => (settings.get(key) ? "on" : "off");
  return [
    { title: "Run program", group: "Command", keys: "Mod+Enter", run: run },
    { title: "Stop program", group: "Command", run: stop },
    { title: "Benchmark (20 runs)", group: "Command", run: benchmark },
    { title: "Format document", group: "Command", keys: "Shift+Alt+F", run: () => editor.format() },
    { title: "Go to symbol…", group: "Command", keys: "Mod+Shift+O", run: () => openPalette(symbolItems(), "Go to symbol") },
    { title: "Share project…", group: "Command", run: share },
    { title: "New file…", group: "File", run: newFile },
    { title: "Rename current file…", group: "File", run: () => renameActive() },
    { title: "Delete current file…", group: "File", run: () => project.active !== ENTRY_FILE && deleteFile(project.active) },
    { title: "Import files…", group: "File", run: pickFiles },
    { title: "Download current file", group: "File", run: exportFile },
    { title: "Download project as .zip", group: "File", run: exportZip },
    { title: "Copy output", group: "Command", run: () => copyText(views.outputText, "Output copied") },
    { title: "Clear output", group: "Command", run: () => views.clearOutput() },
    ...THEMES.map((theme) => ({
      title: `Theme: ${theme[0].toUpperCase()}${theme.slice(1)}`,
      group: "Setting",
      detail: settings.get("theme") === theme ? "current" : "",
      run: () => settings.set("theme", theme),
    })),
    { title: `Toggle run on change (${flag("autorun")})`, group: "Setting", run: () => toggle("autorun", "Run on change") },
    { title: `Toggle Vim keys (${flag("vim")})`, group: "Setting", run: () => toggle("vim", "Vim keys") },
    { title: `Toggle word wrap (${flag("wrap")})`, group: "Setting", run: () => toggle("wrap", "Word wrap") },
    { title: "Toggle stacked layout", group: "Setting", run: () => toggle("stacked", "Stacked layout") },
    { title: "Increase font size", group: "Setting", run: () => changeFontSize(1) },
    { title: "Decrease font size", group: "Setting", run: () => changeFontSize(-1) },
    { title: "Keyboard shortcuts", group: "Help", run: showShortcuts },
    {
      title: "Open the language guide",
      group: "Help",
      run: () => window.open("https://github.com/nandan-varma/meadows/blob/main/docs/LANGUAGE.md", "_blank", "noopener"),
    },
    ...viewCommands(),
  ];
}

function paletteItems() {
  return [
    ...commandItems(),
    ...project.files.map((file) => ({
      title: file.name,
      group: "File",
      detail: file.name === project.active ? "open" : "",
      run: () => openFile(file.name),
    })),
    ...symbolItems(),
    ...EXAMPLES.map((example) => ({
      title: example.title,
      group: "Example",
      run: () => chooseExample(example),
    })),
  ];
}

function openCommandPalette() {
  openPalette(paletteItems());
}

function setupDivider() {
  const move = (event) => {
    const rect = dom.workspace.getBoundingClientRect();
    const stacked = dom.workspace.classList.contains("stacked");
    const ratio = stacked
      ? (event.clientY - rect.top) / rect.height
      : (event.clientX - rect.left) / rect.width;
    settings.set("split", Math.min(0.8, Math.max(0.2, Math.round(ratio * 1000) / 1000)));
  };
  dom.divider.addEventListener("pointerdown", (event) => {
    dom.divider.setPointerCapture(event.pointerId);
    dom.divider.classList.add("dragging");
    dom.workspace.classList.add("dragging");
    dom.divider.addEventListener("pointermove", move);
  });
  const release = (event) => {
    dom.divider.removeEventListener("pointermove", move);
    dom.divider.classList.remove("dragging");
    dom.workspace.classList.remove("dragging");
    dom.divider.releasePointerCapture?.(event.pointerId);
  };
  dom.divider.addEventListener("pointerup", release);
  dom.divider.addEventListener("pointercancel", release);
  dom.divider.addEventListener("dblclick", () => settings.set("split", 0.55));
  dom.divider.addEventListener("keydown", (event) => {
    const step = { ArrowLeft: -0.02, ArrowUp: -0.02, ArrowRight: 0.02, ArrowDown: 0.02 }[event.key];
    if (step) {
      event.preventDefault();
      settings.set("split", Math.min(0.8, Math.max(0.2, Math.round((settings.get("split") + step) * 1000) / 1000)));
    }
  });
}

function setupTablist(list) {
  list.addEventListener("keydown", (event) => {
    const tabs = [...list.querySelectorAll("[role=tab]")];
    const index = tabs.indexOf(document.activeElement);
    const step = { ArrowLeft: -1, ArrowRight: 1 }[event.key];
    if (index >= 0 && step) {
      event.preventDefault();
      tabs[(index + step + tabs.length) % tabs.length].focus();
      tabs[(index + step + tabs.length) % tabs.length].click();
    }
  });
}

function setupDropZone() {
  let depth = 0;
  const hasFiles = (event) => event.dataTransfer?.types.includes("Files");
  window.addEventListener("dragenter", (event) => {
    if (hasFiles(event)) {
      depth++;
      dom.drop.hidden = false;
    }
  });
  window.addEventListener("dragleave", () => {
    depth = Math.max(0, depth - 1);
    dom.drop.hidden = depth === 0;
  });
  window.addEventListener("dragover", (event) => hasFiles(event) && event.preventDefault());
  window.addEventListener("drop", (event) => {
    if (!hasFiles(event)) {
      return;
    }
    event.preventDefault();
    depth = 0;
    dom.drop.hidden = true;
    importFiles([...event.dataTransfer.files]);
  });
}

function setupShortcuts() {
  window.addEventListener(
    "keydown",
    (event) => {
      const mod = isMac() ? event.metaKey : event.ctrlKey;
      if (mod && event.key.toLowerCase() === "k") {
        event.preventDefault();
        event.stopPropagation();
        openCommandPalette();
      } else if (mod && event.shiftKey && event.key.toLowerCase() === "o") {
        event.preventDefault();
        event.stopPropagation();
        openPalette(symbolItems(), "Go to symbol");
      } else if (mod && event.key === "Enter") {
        event.preventDefault();
        event.stopPropagation();
        run();
      }
    },
    true,
  );
}

async function initialProject() {
  if (location.hash.startsWith(SHARE_PREFIX)) {
    const shared = await decodeProject(location.hash.slice(SHARE_PREFIX.length));
    history.replaceState(null, "", location.pathname + location.search);
    if (shared) {
      return shared;
    }
    toast("The shared link could not be read", "error");
  }
  return loadSavedProject() ?? createProject(findExample(DEFAULT_EXAMPLE).files);
}

function registerServiceWorker() {
  if (!("serviceWorker" in navigator) || !window.isSecureContext) {
    return;
  }
  const hadController = Boolean(navigator.serviceWorker.controller);
  navigator.serviceWorker.register("sw.js").catch(() => {});
  navigator.serviceWorker.addEventListener("controllerchange", () => {
    if (hadController) {
      toast("Playground updated. Reload for the latest version.");
    } else {
      toast("Ready to work offline");
    }
  });
}

$("palette-keys").replaceChildren(kbd("Mod+K"));
$("run-keys").replaceChildren(kbd("Mod+Enter"));
$("hint-keys").replaceChildren(kbd("Mod+Enter"));
dom.run.addEventListener("click", () => run());
dom.stop.addEventListener("click", stop);
dom.share.addEventListener("click", share);
dom.theme.addEventListener("click", cycleTheme);
dom.layout.addEventListener("click", () => toggle("stacked", "Stacked layout"));
dom.addFile.addEventListener("click", newFile);
$("open-palette").addEventListener("click", openCommandPalette);
$("copy-output").addEventListener("click", () => copyText(views.outputText, "Output copied"));
$("clear-output").addEventListener("click", () => views.clearOutput());
$("expand-tree").addEventListener("click", () => views.setTreeExpanded(true));
$("collapse-tree").addEventListener("click", () => views.setTreeExpanded(false));
dom.views.addEventListener("click", (event) => {
  const tab = event.target.closest("[role=tab]");
  if (tab) {
    views.show(tab.dataset.view);
  }
});
setupTablist(dom.views);
setupTablist(dom.files);
setupDivider();
setupDropZone();
setupShortcuts();

settings.subscribe(applySettings);
NARROW.addEventListener("change", () => applySettings());
DARK.addEventListener("change", () => applySettings());

loadProject(await initialProject());
analyzer.version.then((version) => {
  dom.version.textContent = `v${version}`;
  dom.version.hidden = false;
});
setStatus("Ready");
registerServiceWorker();
