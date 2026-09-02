import createMeadowsModule from "./meadows.js";
// Unpinned major versions on purpose — see the comment in meadows-lang.js.
import { EditorState } from "https://esm.sh/@codemirror/state@6";
import {
  EditorView,
  keymap,
  lineNumbers,
  highlightActiveLine,
  highlightActiveLineGutter,
  drawSelection,
  dropCursor,
} from "https://esm.sh/@codemirror/view@6";
import { bracketMatching, indentOnInput } from "https://esm.sh/@codemirror/language@6";
import { defaultKeymap, history, historyKeymap, indentWithTab } from "https://esm.sh/@codemirror/commands@6";
import { meadowsSetup } from "./meadows-lang.js";

const editorEl = document.getElementById("editor");
const runBtn = document.getElementById("run-btn");
const statusEl = document.getElementById("status");
const panels = {
  output: document.getElementById("output"),
  diagnostics: document.getElementById("diagnostics"),
  tokens: document.getElementById("tokens"),
  ast: document.getElementById("ast"),
};

const STORAGE_KEY = "meadows-playground-source";

const DEFAULT_SOURCE = `func fib(n) {
  if (n <= 1) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

for (i in range(0, 8)) {
  print(fib(i));
}
`;

document.querySelectorAll(".tab-btn").forEach((btn) => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".tab-btn").forEach((b) => {
      b.classList.remove("active");
      b.setAttribute("aria-selected", "false");
    });
    document.querySelectorAll(".tab-panel").forEach((p) => p.classList.remove("active"));
    btn.classList.add("active");
    btn.setAttribute("aria-selected", "true");
    panels[btn.dataset.tab].classList.add("active");
  });
});

function render(result) {
  panels.diagnostics.textContent = result.diagnostics || "No diagnostics.";
  panels.tokens.textContent = result.tokens || "(no tokens)";
  panels.ast.textContent = result.ast || "(no AST — parsing failed)";

  if (!result.success) {
    panels.output.textContent = "(not run — fix the errors in Diagnostics first)";
    statusEl.textContent = "Analysis found errors — see Diagnostics.";
    return;
  }

  if (!result.ran) {
    panels.output.textContent = "(nothing to run)";
    statusEl.textContent = "Analyzed successfully — nothing to run.";
    return;
  }

  panels.output.textContent = result.output || "(program produced no output)";
  statusEl.textContent =
    result.exitCode === 0
      ? "Ran successfully."
      : `Program exited with code ${result.exitCode} — see Output.`;
}

function loadSavedSource() {
  try {
    return localStorage.getItem(STORAGE_KEY);
  } catch {
    return null; // private browsing / storage disabled — not fatal
  }
}

function saveSource(text) {
  try {
    localStorage.setItem(STORAGE_KEY, text);
  } catch {
    // best-effort only
  }
}

async function main() {
  let Module;
  try {
    Module = await createMeadowsModule();
  } catch (err) {
    statusEl.textContent =
      "The Meadows compiler could not be loaded. This usually means the WebAssembly files are not available or failed to initialize.";
    console.error(err);
    return;
  }

  statusEl.textContent = "Ready.";

  const run = () => {
    try {
      render(Module.compileSource(view.state.doc.toString()));
    } catch (err) {
      statusEl.textContent = "Internal error while running source.";
      console.error(err);
    }
  };

  const persist = EditorView.updateListener.of((update) => {
    if (update.docChanged) saveSource(update.state.doc.toString());
  });

  const runKeymap = keymap.of([
    { key: "Mod-Enter", run: () => (run(), true) },
    indentWithTab,
    ...defaultKeymap,
    ...historyKeymap,
  ]);

  const view = new EditorView({
    parent: editorEl,
    state: EditorState.create({
      doc: loadSavedSource() || DEFAULT_SOURCE,
      extensions: [
        lineNumbers(),
        highlightActiveLineGutter(),
        highlightActiveLine(),
        drawSelection(),
        dropCursor(),
        bracketMatching(),
        indentOnInput(),
        history(),
        EditorView.lineWrapping,
        runKeymap,
        ...meadowsSetup(),
        persist,
      ],
    }),
  });

  runBtn.addEventListener("click", run);
  run();
}

main();
