import { el, icon } from "./dom.js";
import { BUILTIN_DOCS, KEYWORD_DOCS, LIMITS, OPERATORS } from "./docs.js";
import { parseBytecode, parseTokens, parseTree, splitLocations } from "./parse.js";

const TOKEN_LIMIT = 2000;
const OPEN_DEPTH = 3;
const OP_CLASSES = {
  flow: ["Jump", "JumpIfFalse", "AndShort", "OrShort", "ForNext", "ForInit", "ForStep", "Halt"],
  call: ["Call", "CallBuiltin", "Return"],
  data: ["Constant", "Load", "Store", "Assign", "LoadFunction", "Clear", "Pop"],
  collection: ["MakeArray", "MakeObject", "Index", "SetIndex", "Field", "SetField"],
};

function opClass(op) {
  const found = Object.entries(OP_CLASSES).find(([, ops]) => ops.includes(op));
  return found ? found[0] : "math";
}

function tokenClass(type) {
  if (/^(LET|FUNC|IF|ELSE|FOR|WHILE|RETURN|IN|RANGE|BREAK|CONTINUE|IMPORT|TRUE|FALSE)$/.test(type)) {
    return "keyword";
  }
  if (/^(STRING|NUMBER|INTEGER|FLOAT)/.test(type)) {
    return "literal";
  }
  if (type === "IDENTIFIER") {
    return "name";
  }
  return "symbol";
}

export class Views {
  #root;
  #handlers;
  #output;
  #outputEmpty;
  #meta;
  #outputText = "";

  constructor(root, handlers) {
    this.#root = root;
    this.#handlers = handlers;
    this.#output = root.querySelector("#output");
    this.#outputEmpty = root.querySelector("#output-empty");
    this.#meta = root.querySelector("#run-meta");
    this.#renderReference();
  }

  show(name) {
    for (const tab of this.#root.querySelectorAll("[role=tab]")) {
      const selected = tab.dataset.view === name;
      tab.setAttribute("aria-selected", String(selected));
      tab.tabIndex = selected ? 0 : -1;
    }
    for (const panel of this.#root.querySelectorAll(".view")) {
      panel.hidden = panel.dataset.view !== name;
    }
  }

  get current() {
    return this.#root.querySelector("[role=tab][aria-selected=true]").dataset.view;
  }

  get outputText() {
    return this.#outputText;
  }

  clearOutput() {
    this.#output.replaceChildren();
    this.#outputText = "";
    this.#outputEmpty.hidden = false;
    this.#meta.replaceChildren();
  }

  #linked(text) {
    const names = new Set(this.#handlers.fileNames());
    return splitLocations(text, names).map((segment) =>
      segment.file
        ? el(
            "button",
            {
              type: "button",
              class: "location",
              title: "Go to this location",
              onclick: () => this.#handlers.onNavigate(segment.file, segment.line, segment.column),
            },
            segment.text,
          )
        : segment.text,
    );
  }

  #append(text, className) {
    if (!text) {
      return;
    }
    const nearBottom =
      this.#output.scrollHeight - this.#output.scrollTop - this.#output.clientHeight < 40;
    this.#outputEmpty.hidden = true;
    this.#output.append(el("span", { class: className }, ...this.#linked(text)));
    this.#outputText += text;
    if (nearBottom) {
      this.#output.scrollTop = this.#output.scrollHeight;
    }
  }

  appendOutput(text) {
    this.#append(text, "");
  }

  appendError(text) {
    this.#append(text, "error");
  }

  appendNote(text) {
    this.#append(text, "note");
  }

  setMeta(chips) {
    this.#meta.replaceChildren(
      ...chips.map(({ label, kind }) => el("span", { class: `chip ${kind ?? ""}` }, label)),
    );
  }

  renderAnalysis(report, diagnostics) {
    this.#renderProblems(diagnostics);
    this.#renderTokens(report.tokens);
    this.#renderTree(report.ast);
    this.#renderBytecode(report.bytecode);
  }

  #renderProblems(diagnostics) {
    const panel = this.#root.querySelector("#problems");
    if (diagnostics.length === 0) {
      panel.replaceChildren(el("p", { class: "empty-state" }, icon("check"), "No problems found"));
      return;
    }
    panel.replaceChildren(
      el(
        "ul",
        { class: "problem-list" },
        ...diagnostics.map((diagnostic) => {
          const { line, character } = diagnostic.range.start;
          return el(
            "li",
            {},
            el(
              "button",
              {
                type: "button",
                class: `problem ${diagnostic.severity === 1 ? "error" : "warning"}`,
                onclick: () =>
                  this.#handlers.onNavigate(diagnostic.file, line + 1, character + 1),
              },
              el("span", { class: "dot" }),
              el("span", { class: "message" }, diagnostic.message),
              el("span", { class: "code" }, diagnostic.code),
              el("span", { class: "where" }, `${diagnostic.file}:${line + 1}:${character + 1}`),
            ),
          );
        }),
      ),
    );
  }

  #renderTokens(text) {
    const panel = this.#root.querySelector("#tokens");
    const tokens = parseTokens(text);
    const shown = tokens.slice(0, TOKEN_LIMIT);
    panel.replaceChildren(
      el(
        "table",
        { class: "grid tokens" },
        el("thead", {}, el("tr", {}, el("th", {}, "Position"), el("th", {}, "Type"), el("th", {}, "Text"))),
        el(
          "tbody",
          {},
          ...shown.map((token) =>
            el(
              "tr",
              {},
              el("td", { class: "muted" }, `${token.line}:${token.column}`),
              el("td", {}, el("span", { class: `token ${tokenClass(token.type)}` }, token.type)),
              el("td", { class: "mono" }, token.text),
            ),
          ),
        ),
      ),
      tokens.length > shown.length
        ? el("p", { class: "footnote" }, `Showing the first ${TOKEN_LIMIT} of ${tokens.length} tokens`)
        : null,
    );
  }

  #renderTree(text) {
    const panel = this.#root.querySelector("#ast");
    const toolbar = this.#root.querySelector("#ast-tools");
    const roots = parseTree(text);
    toolbar.hidden = roots.length === 0;
    if (roots.length === 0) {
      panel.replaceChildren(
        el("p", { class: "empty-state" }, "The syntax tree is available once the program has no errors."),
      );
      return;
    }
    const build = (node, depth) => {
      if (node.children.length === 0) {
        return el("div", { class: "leaf" }, el("span", { class: "label" }, node.label));
      }
      return el(
        "details",
        { open: depth < OPEN_DEPTH },
        el("summary", {}, el("span", { class: "label" }, node.label), el("span", { class: "count" }, node.children.length)),
        ...node.children.map((child) => build(child, depth + 1)),
      );
    };
    panel.replaceChildren(el("div", { class: "tree" }, ...roots.map((root) => build(root, 0))));
  }

  setTreeExpanded(expanded) {
    for (const details of this.#root.querySelectorAll("#ast details")) {
      details.open = expanded;
    }
  }

  #renderBytecode(text) {
    const panel = this.#root.querySelector("#bytecode");
    const functions = parseBytecode(text);
    if (functions.length === 0) {
      panel.replaceChildren(
        el("p", { class: "empty-state" }, "The bytecode is available once the program has no errors."),
      );
      return;
    }
    panel.replaceChildren(
      ...functions.map((fn) =>
        el(
          "section",
          { class: "listing" },
          el(
            "h3",
            {},
            fn.name,
            el("span", { class: "muted" }, `${fn.params} params · ${fn.slots} slots · ${fn.rows.length} instructions`),
          ),
          el(
            "table",
            { class: "grid code" },
            el(
              "tbody",
              {},
              ...fn.rows.map((row) =>
                el(
                  "tr",
                  {},
                  el("td", { class: "muted" }, String(row.index)),
                  el("td", {}, el("span", { class: `op ${opClass(row.op)}` }, row.op)),
                  el("td", { class: "mono" }, row.operands),
                  el("td", { class: "muted" }, row.line ? `line ${row.line}` : ""),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }

  #renderReference() {
    const panel = this.#root.querySelector("#reference");
    const search = el("input", {
      type: "search",
      placeholder: "Filter the reference",
      "aria-label": "Filter the reference",
      spellcheck: "false",
    });
    const insertable = (snippet) =>
      el(
        "button",
        { type: "button", class: "insert", title: "Insert at the cursor", onclick: () => this.#handlers.onInsert(snippet) },
        icon("plus"),
        "Insert",
      );

    const entries = [
      ...Object.entries(BUILTIN_DOCS).map(([name, doc]) => ({
        group: "Built-in",
        name,
        node: el(
          "article",
          {},
          el("code", { class: "signature" }, doc.signature),
          el("p", {}, doc.summary),
          el("pre", {}, doc.example),
          insertable(doc.example),
        ),
      })),
      ...Object.entries(KEYWORD_DOCS).map(([name, doc]) => ({
        group: "Keyword",
        name,
        node: el(
          "article",
          {},
          el("code", { class: "signature" }, name),
          el("p", {}, doc.summary),
          el("pre", {}, doc.example),
          insertable(doc.example),
        ),
      })),
      ...OPERATORS.map(([name, summary]) => ({
        group: "Operator",
        name,
        node: el("article", { class: "row" }, el("code", { class: "signature" }, name), el("p", {}, summary)),
      })),
      ...LIMITS.map(([name, summary]) => ({
        group: "Limit",
        name,
        node: el("article", { class: "row" }, el("code", { class: "signature" }, name), el("p", {}, summary)),
      })),
    ];

    const body = el("div", { class: "reference" });
    const paint = () => {
      const query = search.value.trim().toLowerCase();
      const matching = entries.filter((entry) =>
        `${entry.name} ${entry.node.textContent}`.toLowerCase().includes(query),
      );
      body.replaceChildren(
        ...["Built-in", "Keyword", "Operator", "Limit"].flatMap((group) => {
          const inGroup = matching.filter((entry) => entry.group === group);
          return inGroup.length === 0
            ? []
            : [el("h3", {}, group === "Built-in" ? "Built-in functions" : `${group}s`), ...inGroup.map((entry) => entry.node)];
        }),
        matching.length === 0 ? el("p", { class: "empty-state" }, "Nothing matches") : null,
        el(
          "a",
          {
            class: "guide-link",
            href: "https://github.com/nandan-varma/meadows/blob/main/docs/LANGUAGE.md",
            target: "_blank",
            rel: "noopener",
          },
          "Read the full language guide",
        ),
      );
    };
    search.addEventListener("input", paint);
    paint();
    panel.replaceChildren(el("div", { class: "search-row" }, search), body);
  }
}
