import { EXAMPLES, findExample } from "./examples.js";
import { Editor } from "./editor.js";
import {
  ENTRY_FILE,
  activeFile,
  addFile,
  createProject,
  decodeProject,
  encodeProject,
  loadSavedProject,
  removeFile,
  saveProject,
} from "./project.js";
import { Analyzer, Runner } from "./runner.js";

const ANALYSIS_DELAY_MS = 300;
const SHARE_PREFIX = "#project=";
const DEFAULT_EXAMPLE = "fibonacci";

const $ = (id) => document.getElementById(id);
const dom = {
  files: $("files"),
  newFileForm: $("new-file-form"),
  newFile: $("new-file"),
  addFile: $("add-file"),
  examples: $("examples"),
  run: $("run"),
  stop: $("stop"),
  share: $("share"),
  views: $("views"),
  output: $("output"),
  problems: $("problems"),
  tokens: $("tokens"),
  ast: $("ast"),
  problemCount: $("problem-count"),
  status: $("status"),
  cursor: $("cursor"),
  version: $("version"),
};

const runner = new Runner();
const analyzer = new Analyzer();
const editor = new Editor($("editor"), {
  onChange: handleEdit,
  onRun: run,
  onCursor: (line, column) => {
    dom.cursor.textContent = `Ln ${line}, Col ${column}`;
  },
});

let project;
let edited = false;
let diagnostics = [];
let analysisTimer;

function setStatus(text) {
  dom.status.textContent = text;
}

function describeProblems() {
  const errors = diagnostics.filter((d) => d.severity === 1).length;
  const warnings = diagnostics.length - errors;
  if (diagnostics.length === 0) {
    return "No problems";
  }
  const plural = (count, word) => `${count} ${word}${count === 1 ? "" : "s"}`;
  return `${plural(errors, "error")}, ${plural(warnings, "warning")}`;
}

function showView(name) {
  for (const tab of dom.views.querySelectorAll("[role=tab]")) {
    tab.setAttribute("aria-selected", String(tab.dataset.view === name));
  }
  for (const view of ["output", "problems", "tokens", "ast"]) {
    dom[view].hidden = view !== name;
  }
}

function appendOutput(text, className) {
  if (!text) {
    return;
  }
  const nearBottom =
    dom.output.scrollHeight - dom.output.scrollTop - dom.output.clientHeight < 40;
  const node = document.createElement("span");
  if (className) {
    node.className = className;
  }
  node.textContent = text;
  dom.output.append(node);
  if (nearBottom) {
    dom.output.scrollTop = dom.output.scrollHeight;
  }
}

function renderFiles() {
  dom.files.replaceChildren(
    ...project.files.map((file) => {
      const group = document.createElement("span");
      group.className = "file-tab-group";

      const tab = document.createElement("button");
      tab.type = "button";
      tab.className = "file-tab";
      tab.setAttribute("role", "tab");
      tab.setAttribute("aria-selected", String(file.name === project.active));
      tab.textContent = file.name;
      tab.addEventListener("click", () => openFile(file.name));
      group.append(tab);

      if (file.name !== ENTRY_FILE) {
        const close = document.createElement("button");
        close.type = "button";
        close.className = "file-close";
        close.textContent = "×";
        close.title = `Delete ${file.name}`;
        close.setAttribute("aria-label", `Delete ${file.name}`);
        close.addEventListener("click", () => deleteFile(file.name));
        group.append(close);
      }
      return group;
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

function deleteFile(name) {
  try {
    removeFile(project, name);
  } catch (error) {
    setStatus(error.message);
    return;
  }
  editor.forget(name);
  openFile(project.active);
  scheduleAnalysis(0);
}

function loadProject(next) {
  project = next;
  edited = false;
  diagnostics = [];
  editor.reset();
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
  dom.problems.textContent = report.diagnostics || "No problems found.\n";
  dom.tokens.textContent = report.tokens;
  dom.ast.textContent =
    report.ast || "The syntax tree is available once the program has no errors.\n";

  const errors = diagnostics.filter((d) => d.severity === 1).length;
  dom.problemCount.hidden = diagnostics.length === 0;
  dom.problemCount.textContent = String(diagnostics.length);
  dom.problemCount.classList.toggle("warnings-only", errors === 0);

  showDiagnosticsForActiveFile();
  if (!runner.running) {
    setStatus(describeProblems());
  }
}

function setRunning(running) {
  dom.run.disabled = running;
  dom.stop.disabled = !running;
}

function formatDuration(milliseconds) {
  return milliseconds < 1000
    ? `${Math.max(1, Math.round(milliseconds))} ms`
    : `${(milliseconds / 1000).toFixed(2)} s`;
}

function finishRun(result) {
  setRunning(false);
  const took = formatDuration(result.elapsed);

  if (result.status === "stopped") {
    appendOutput("\nStopped.\n", "note");
    setStatus(`Stopped after ${took}`);
  } else if (result.status === "limit") {
    appendOutput("\nOutput limit reached; execution stopped.\n", "error");
    setStatus("Stopped: output limit reached");
  } else if (result.status === "failure") {
    appendOutput(`\n${result.message}\n`, "error");
    setStatus("The program could not be run");
  } else if (!result.report.compiled) {
    appendOutput(result.report.diagnostics, "error");
    appendOutput("\nFix the problems above to run the program.\n", "note");
    setStatus(describeProblems());
  } else if (result.report.exitCode !== 0) {
    appendOutput(result.report.runtimeError, "error");
    setStatus(`Failed after ${took}`);
  } else {
    setStatus(`Finished in ${took}`);
  }
}

function run() {
  if (runner.running) {
    return;
  }
  dom.output.replaceChildren();
  showView("output");
  setRunning(true);
  setStatus("Running…");
  runner.run(project, ENTRY_FILE, {
    onOutput: (text) => appendOutput(text),
    onFinish: finishRun,
  });
}

async function share() {
  const url = `${location.origin}${location.pathname}${SHARE_PREFIX}${await encodeProject(project)}`;
  try {
    await navigator.clipboard.writeText(url);
    setStatus("Link to this project copied to the clipboard");
  } catch {
    history.replaceState(null, "", url);
    setStatus("Copy the link from the address bar to share this project");
  }
}

function chooseExample(id) {
  const example = findExample(id);
  dom.examples.value = "";
  if (!example) {
    return;
  }
  if (edited && !confirm("Replace your project with this example?")) {
    return;
  }
  loadProject(createProject(example.files));
  history.replaceState(null, "", location.pathname);
}

function showNewFileInput(visible) {
  dom.newFile.hidden = !visible;
  dom.addFile.hidden = visible;
  if (visible) {
    dom.newFile.value = "";
    dom.newFile.focus();
  }
}

function submitNewFile(event) {
  event.preventDefault();
  let name = dom.newFile.value.trim();
  if (name && !name.includes(".")) {
    name += ".ms";
  }
  try {
    addFile(project, name);
  } catch (error) {
    setStatus(error.message);
    return;
  }
  showNewFileInput(false);
  renderFiles();
  openFile(name);
  scheduleAnalysis(0);
}

async function initialProject() {
  if (location.hash.startsWith(SHARE_PREFIX)) {
    const shared = await decodeProject(location.hash.slice(SHARE_PREFIX.length));
    if (shared) {
      return shared;
    }
  }
  return loadSavedProject() ?? createProject(findExample(DEFAULT_EXAMPLE).files);
}

dom.examples.append(new Option("Examples…", ""));
for (const example of EXAMPLES) {
  dom.examples.append(new Option(example.title, example.id));
}
dom.examples.addEventListener("change", () => chooseExample(dom.examples.value));
dom.run.addEventListener("click", run);
dom.stop.addEventListener("click", () => runner.stop());
dom.share.addEventListener("click", share);
dom.addFile.addEventListener("click", () => showNewFileInput(true));
dom.newFileForm.addEventListener("submit", submitNewFile);
dom.newFile.addEventListener("keydown", (event) => {
  if (event.key === "Escape") {
    showNewFileInput(false);
  }
});
dom.views.addEventListener("click", (event) => {
  const tab = event.target.closest("[role=tab]");
  if (tab) {
    showView(tab.dataset.view);
  }
});

loadProject(await initialProject());
analyzer.version.then((version) => {
  dom.version.textContent = `v${version}`;
  dom.version.hidden = false;
});
setStatus("Ready");
