import {
  autocompletion,
  closeBrackets,
  closeBracketsKeymap,
  completionKeymap,
  snippetCompletion,
} from "@codemirror/autocomplete";
import { defaultKeymap, history, historyKeymap, indentWithTab } from "@codemirror/commands";
import { bracketMatching, foldGutter, foldKeymap, foldService, indentOnInput } from "@codemirror/language";
import { lintGutter, setDiagnostics } from "@codemirror/lint";
import { highlightSelectionMatches, search, searchKeymap } from "@codemirror/search";
import { Compartment, EditorState, StateEffect, StateField } from "@codemirror/state";
import {
  Decoration,
  EditorView,
  drawSelection,
  highlightActiveLine,
  highlightActiveLineGutter,
  hoverTooltip,
  keymap,
  lineNumbers,
} from "@codemirror/view";
import { BUILTIN_DOCS, KEYWORD_DOCS, SNIPPETS } from "./docs.js";
import { formatSource } from "./format.js";
import { meadows } from "./language.js";
import { findLocals } from "./symbols.js";

const FLASH_MS = 1400;
const WORD = /[A-Za-z_][A-Za-z0-9_]*/;

const flash = StateEffect.define();
const clearFlash = StateEffect.define();

const flashField = StateField.define({
  create: () => Decoration.none,
  update(decorations, transaction) {
    for (const effect of transaction.effects) {
      if (effect.is(flash)) {
        return Decoration.set([Decoration.line({ class: "cm-flash" }).range(effect.value)]);
      }
      if (effect.is(clearFlash)) {
        return Decoration.none;
      }
    }
    return decorations.map(transaction.changes);
  },
  provide: (field) => EditorView.decorations.from(field),
});

const braceFolding = foldService.of((state, from) => {
  const line = state.doc.lineAt(from);
  const opening = line.text.lastIndexOf("{");
  if (opening < 0 || line.text.slice(opening + 1).trim() !== "") {
    return null;
  }
  let depth = 0;
  for (let number = line.number; number <= state.doc.lines; number++) {
    const text = state.doc.line(number).text;
    for (let i = number === line.number ? opening : 0; i < text.length; i++) {
      depth += text[i] === "{" ? 1 : text[i] === "}" ? -1 : 0;
      if (depth === 0 && number > line.number) {
        return { from: line.to, to: state.doc.line(number).from + i };
      }
    }
  }
  return null;
});

const theme = EditorView.theme({
  "&": { height: "100%", color: "var(--text)", backgroundColor: "transparent" },
  ".cm-scroller": {
    fontFamily: "var(--mono)",
    fontSize: "var(--editor-font-size)",
    lineHeight: "1.65",
  },
  ".cm-content": { caretColor: "var(--accent)", padding: "0.75rem 0" },
  ".cm-cursor": { borderLeftColor: "var(--accent)", borderLeftWidth: "2px" },
  "&.cm-focused": { outline: "none" },
  ".cm-gutters": { backgroundColor: "transparent", color: "var(--faint)", border: "none" },
  ".cm-lineNumbers .cm-gutterElement": { padding: "0 0.75rem 0 1rem" },
  ".cm-activeLine": { backgroundColor: "var(--active-line)" },
  ".cm-activeLineGutter": { backgroundColor: "transparent", color: "var(--text)" },
  "&.cm-focused .cm-selectionBackground, .cm-selectionBackground, ::selection": {
    backgroundColor: "var(--selection)",
  },
  ".cm-matchingBracket": { backgroundColor: "var(--selection)", outline: "none" },
  ".cm-selectionMatch": { backgroundColor: "var(--selection-match)" },
  ".cm-foldPlaceholder": {
    backgroundColor: "var(--hover)",
    border: "none",
    color: "var(--muted)",
    padding: "0 0.4rem",
    borderRadius: "4px",
  },
});

function documentFor(word, symbols) {
  const symbol = symbols().find((candidate) => candidate.name === word);
  if (symbol) {
    return {
      signature: `${symbol.kind === "function" ? "func " : "let "}${symbol.signature}`,
      summary: `Defined in ${symbol.file}:${symbol.line}`,
    };
  }
  if (BUILTIN_DOCS[word]) {
    return BUILTIN_DOCS[word];
  }
  if (KEYWORD_DOCS[word]) {
    return { signature: word, ...KEYWORD_DOCS[word] };
  }
  return null;
}

function documentNode({ signature, summary, example }) {
  const node = document.createElement("div");
  node.className = "doc-tip";
  const head = document.createElement("code");
  head.textContent = signature;
  const body = document.createElement("p");
  body.textContent = summary;
  node.append(head, body);
  if (example) {
    const sample = document.createElement("pre");
    sample.textContent = example;
    node.append(sample);
  }
  return node;
}

export class Editor {
  #handlers;
  #states = new Map();
  #current = null;
  #view;
  #vim = new Compartment();
  #wrap = new Compartment();
  #vimExtension = null;
  #options = { vim: false, wrap: false };
  #flashTimer;

  constructor(parent, handlers) {
    this.#handlers = handlers;
    this.#view = new EditorView({ parent, state: this.#createState("") });
  }

  #complete = (context) => {
    const word = context.matchBefore(WORD);
    if (!word || (word.from === word.to && !context.explicit)) {
      return null;
    }
    const options = [
      ...Object.entries(KEYWORD_DOCS).map(([label, doc]) => ({
        label,
        type: "keyword",
        info: doc.summary,
      })),
      ...Object.entries(BUILTIN_DOCS).map(([label, doc]) => ({
        label,
        type: "function",
        detail: doc.signature.slice(label.length),
        info: doc.summary,
        boost: 2,
      })),
      ...SNIPPETS.map((snippet) =>
        snippetCompletion(snippet.template, {
          label: snippet.label,
          detail: snippet.detail,
          type: "text",
          boost: -1,
        }),
      ),
      ...this.#handlers.symbols().map((symbol) => ({
        label: symbol.name,
        type: symbol.kind,
        detail: symbol.kind === "function" ? symbol.signature.slice(symbol.name.length) : "",
        info: `${symbol.file}:${symbol.line}`,
        boost: 3,
      })),
      ...findLocals(context.state.doc.toString()).map((label) => ({
        label,
        type: "variable",
        boost: 1,
      })),
    ];
    const unique = new Map(options.map((option) => [option.label + option.type, option]));
    return { from: word.from, options: [...unique.values()], validFor: /^[A-Za-z0-9_]*$/ };
  };

  #hover = hoverTooltip((view, position) => {
    const range = view.state.wordAt(position);
    if (!range) {
      return null;
    }
    const doc = documentFor(view.state.sliceDoc(range.from, range.to), this.#handlers.symbols);
    return doc
      ? { pos: range.from, end: range.to, above: true, create: () => ({ dom: documentNode(doc) }) }
      : null;
  });

  #definition = EditorView.domEventHandlers({
    mousedown: (event, view) => {
      if (!(event.metaKey || event.ctrlKey)) {
        return false;
      }
      const position = view.posAtCoords({ x: event.clientX, y: event.clientY });
      return position !== null && this.#goToDefinition(view.state, position);
    },
  });

  #goToDefinition(state, position) {
    const word = state.wordAt(position);
    return word ? this.#handlers.onDefinition(state.sliceDoc(word.from, word.to)) : false;
  }

  #createState(text) {
    return EditorState.create({
      doc: text,
      extensions: [
        this.#vim.of(this.#options.vim ? this.#vimExtension : []),
        this.#wrap.of(this.#options.wrap ? EditorView.lineWrapping : []),
        lineNumbers(),
        lintGutter(),
        foldGutter({ openText: "⌄", closedText: "›" }),
        braceFolding,
        highlightActiveLine(),
        highlightActiveLineGutter(),
        highlightSelectionMatches(),
        drawSelection(),
        history(),
        bracketMatching(),
        closeBrackets(),
        indentOnInput(),
        search({ top: true }),
        autocompletion({ override: [this.#complete], icons: false }),
        this.#hover,
        this.#definition,
        flashField,
        meadows(),
        theme,
        EditorView.contentAttributes.of({ "aria-label": "Source code" }),
        keymap.of([
          { key: "Mod-Enter", run: () => (this.#handlers.onRun(), true) },
          { key: "Mod-s", run: () => (this.format(), true), preventDefault: true },
          { key: "Shift-Alt-f", run: () => (this.format(), true) },
          {
            key: "F12",
            run: (view) => this.#goToDefinition(view.state, view.state.selection.main.head),
          },
          indentWithTab,
          ...closeBracketsKeymap,
          ...completionKeymap,
          ...searchKeymap,
          ...foldKeymap,
          ...defaultKeymap,
          ...historyKeymap,
        ]),
        EditorView.updateListener.of((update) => {
          if (update.docChanged) {
            this.#handlers.onChange(this.#current, update.state.doc.toString());
          }
          if (update.selectionSet || update.docChanged) {
            const { head, from, to } = update.state.selection.main;
            const line = update.state.doc.lineAt(head);
            this.#handlers.onCursor(line.number, head - line.from + 1, to - from);
          }
        }),
      ],
    });
  }

  open(name, text) {
    if (this.#current) {
      this.#states.set(this.#current, this.#view.state);
    }
    this.#current = name;
    this.#view.setState(this.#states.get(name) ?? this.#createState(text));
  }

  reset() {
    this.#states.clear();
    this.#current = null;
  }

  forget(name) {
    this.#states.delete(name);
  }

  rename(from, to) {
    if (this.#current === from) {
      this.#current = to;
    } else if (this.#states.has(from)) {
      this.#states.set(to, this.#states.get(from));
      this.#states.delete(from);
    }
  }

  focus() {
    this.#view.focus();
  }

  async configure({ vim: useVim, wrap }) {
    if (useVim && !this.#vimExtension) {
      const { vim } = await import("@replit/codemirror-vim");
      this.#vimExtension = vim();
    }
    this.#options = { vim: useVim, wrap };
    this.#states.clear();
    this.#view.dispatch({
      effects: [
        this.#vim.reconfigure(useVim ? this.#vimExtension : []),
        this.#wrap.reconfigure(wrap ? EditorView.lineWrapping : []),
      ],
    });
  }

  format() {
    const doc = this.#view.state.doc.toString();
    const formatted = formatSource(doc);
    if (formatted !== doc) {
      this.#view.dispatch({
        changes: { from: 0, to: doc.length, insert: formatted },
        selection: { anchor: Math.min(this.#view.state.selection.main.head, formatted.length) },
      });
    }
  }

  insert(text) {
    this.#view.dispatch(this.#view.state.replaceSelection(text));
    this.#view.focus();
  }

  goTo(line, column = 1) {
    const doc = this.#view.state.doc;
    const target = doc.line(Math.min(Math.max(line, 1), doc.lines));
    const position = Math.min(target.from + Math.max(column - 1, 0), target.to);
    this.#view.dispatch({
      selection: { anchor: position },
      effects: [EditorView.scrollIntoView(position, { y: "center" }), flash.of(target.from)],
    });
    this.#view.focus();
    clearTimeout(this.#flashTimer);
    this.#flashTimer = setTimeout(
      () => this.#view.dispatch({ effects: clearFlash.of(null) }),
      FLASH_MS,
    );
  }

  showDiagnostics(diagnostics) {
    const doc = this.#view.state.doc;
    const marks = diagnostics.map((diagnostic) => {
      const line = doc.line(Math.min(diagnostic.range.start.line + 1, doc.lines));
      const from = Math.min(line.from + diagnostic.range.start.character, line.to);
      const to = Math.max(
        Math.min(line.from + diagnostic.range.end.character, line.to),
        Math.min(from + 1, doc.length),
      );
      return {
        from,
        to,
        severity: diagnostic.severity === 1 ? "error" : "warning",
        message: diagnostic.message,
        source: diagnostic.code,
      };
    });
    this.#view.dispatch(setDiagnostics(this.#view.state, marks));
  }
}
