import createMeadowsModule from "./meadows.js";

const sourceEl = document.getElementById("source");
const runBtn = document.getElementById("run-btn");
const statusEl = document.getElementById("status");
const panels = {
  output: document.getElementById("output"),
  diagnostics: document.getElementById("diagnostics"),
  tokens: document.getElementById("tokens"),
  ast: document.getElementById("ast"),
};

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

  const run = () => {
    try {
      render(Module.compileSource(sourceEl.value));
    } catch (err) {
      statusEl.textContent = "Internal error while running source.";
      console.error(err);
    }
  };

  runBtn.addEventListener("click", run);
  run();
}

main();
