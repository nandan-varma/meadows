import createMeadowsModule from "./meadows.js";

const sourceEl = document.getElementById("source");
const runBtn = document.getElementById("run-btn");
const statusEl = document.getElementById("status");
const panels = {
  diagnostics: document.getElementById("diagnostics"),
  tokens: document.getElementById("tokens"),
  ast: document.getElementById("ast"),
};

const DEFAULT_SOURCE = `func greet(name) {
  print("Hello, " + name);
}

let count = 3;
for (i in range(0, count)) {
  greet("world");
}
`;

document.querySelectorAll(".tab-btn").forEach((btn) => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".tab-btn").forEach((b) => b.classList.remove("active"));
    document.querySelectorAll(".tab-panel").forEach((p) => p.classList.remove("active"));
    btn.classList.add("active");
    panels[btn.dataset.tab].classList.add("active");
  });
});

function render(result) {
  panels.diagnostics.textContent = result.diagnostics || "No diagnostics.";
  panels.tokens.textContent = result.tokens || "(no tokens)";
  panels.ast.textContent = result.ast || "(no AST — parsing failed)";
  statusEl.textContent = result.success
    ? "Analyzed successfully."
    : "Analysis found errors — see Diagnostics.";
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
  sourceEl.value = DEFAULT_SOURCE;

  const analyze = () => {
    try {
      render(Module.compileSource(sourceEl.value));
    } catch (err) {
      statusEl.textContent = "Internal error while analyzing source.";
      console.error(err);
    }
  };

  runBtn.addEventListener("click", analyze);
  analyze();
}

main();
