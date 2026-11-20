import { autocompletion, closeBrackets, closeBracketsKeymap } from "@codemirror/autocomplete";
import { defaultKeymap, history, historyKeymap, indentWithTab } from "@codemirror/commands";
import { bracketMatching, indentOnInput } from "@codemirror/language";
import { lintGutter, setDiagnostics } from "@codemirror/lint";
import { EditorState } from "@codemirror/state";
import {
  EditorView,
  drawSelection,
  highlightActiveLine,
  highlightActiveLineGutter,
  keymap,
  lineNumbers,
} from "@codemirror/view";
import { BUILTINS, KEYWORD_LIST, meadows } from "./language.js";

const COMPLETIONS = [
  ...KEYWORD_LIST.map((label) => ({ label, type: "keyword" })),
  ...BUILTINS.map((label) => ({ label, type: "function" })),
];

function complete(context) {
  const word = context.matchBefore(/[A-Za-z_][A-Za-z0-9_]*/);
  if (!word || (word.from === word.to && !context.explicit)) {
    return null;
  }
  return { from: word.from, options: COMPLETIONS, validFor: /^[A-Za-z0-9_]*$/ };
}

const theme = EditorView.theme({
  "&": { height: "100%", color: "var(--text)", backgroundColor: "var(--editor-bg)" },
  ".cm-scroller": { fontFamily: "var(--mono)", fontSize: "0.875rem", lineHeight: "1.55" },
  ".cm-content": { caretColor: "var(--text)", padding: "0.5rem 0" },
  ".cm-cursor": { borderLeftColor: "var(--text)" },
  "&.cm-focused": { outline: "none" },
  ".cm-gutters": {
    backgroundColor: "var(--editor-bg)",
    color: "var(--muted)",
    border: "none",
  },
  ".cm-activeLine": { backgroundColor: "var(--active-line)" },
  ".cm-activeLineGutter": { backgroundColor: "var(--active-line)", color: "var(--text)" },
  "&.cm-focused .cm-selectionBackground, .cm-selectionBackground, ::selection": {
    backgroundColor: "var(--selection)",
  },
  ".cm-matchingBracket": { backgroundColor: "var(--selection)", outline: "none" },
  ".cm-tooltip": {
    backgroundColor: "var(--panel)",
    color: "var(--text)",
    border: "1px solid var(--border)",
  },
  ".cm-tooltip-autocomplete ul li[aria-selected]": {
    backgroundColor: "var(--selection)",
    color: "var(--text)",
  },
});

export class Editor {
  #handlers;
  #states = new Map();
  #current = null;
  #view;

  constructor(parent, handlers) {
    this.#handlers = handlers;
    this.#view = new EditorView({ parent, state: this.#createState("") });
  }

  #createState(text) {
    return EditorState.create({
      doc: text,
      extensions: [
        lineNumbers(),
        lintGutter(),
        highlightActiveLine(),
        highlightActiveLineGutter(),
        drawSelection(),
        history(),
        bracketMatching(),
        closeBrackets(),
        indentOnInput(),
        autocompletion({ override: [complete] }),
        meadows(),
        theme,
        EditorView.contentAttributes.of({ "aria-label": "Source code" }),
        keymap.of([
          { key: "Mod-Enter", run: () => (this.#handlers.onRun(), true) },
          indentWithTab,
          ...closeBracketsKeymap,
          ...defaultKeymap,
          ...historyKeymap,
        ]),
        EditorView.updateListener.of((update) => {
          if (update.docChanged) {
            this.#handlers.onChange(this.#current, update.state.doc.toString());
          }
          if (update.selectionSet || update.docChanged) {
            const head = update.state.selection.main.head;
            const line = update.state.doc.lineAt(head);
            this.#handlers.onCursor(line.number, head - line.from + 1);
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

  focus() {
    this.#view.focus();
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
