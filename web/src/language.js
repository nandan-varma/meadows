import {
  HighlightStyle,
  StreamLanguage,
  indentUnit,
  syntaxHighlighting,
} from "@codemirror/language";
import { tags as t } from "@lezer/highlight";

const KEYWORDS = new Set([
  "let", "func", "if", "else", "for", "while", "return", "in", "range",
  "break", "continue", "import",
]);
const CONSTANTS = new Set(["true", "false"]);
export const BUILTINS = ["print", "len", "str", "push"];
export const KEYWORD_LIST = [...KEYWORDS, ...CONSTANTS];

const OPENERS = "([{";
const CLOSERS = ")]}";

const tokenTable = {
  keyword: t.keyword,
  constant: t.bool,
  string: t.string,
  number: t.number,
  comment: t.comment,
  operator: t.operator,
  bracket: t.bracket,
  punctuation: t.punctuation,
  call: t.function(t.variableName),
  builtin: t.standard(t.variableName),
  variable: t.variableName,
};

function token(stream, state) {
  if (stream.eatSpace()) {
    return null;
  }
  if (stream.match("#") || stream.match("//")) {
    stream.skipToEnd();
    return "comment";
  }
  if (stream.match(/^"(?:[^"\\]|\\.)*"?/)) {
    return "string";
  }
  if (stream.match(/^\d+\.\d+/) || stream.match(/^\d+/)) {
    return "number";
  }
  if (stream.match(/^[A-Za-z_][A-Za-z0-9_]*/)) {
    const word = stream.current();
    if (KEYWORDS.has(word)) {
      return "keyword";
    }
    if (CONSTANTS.has(word)) {
      return "constant";
    }
    if (BUILTINS.includes(word)) {
      return "builtin";
    }
    return stream.peek() === "(" ? "call" : "variable";
  }
  if (stream.match(/^(==|!=|<=|>=|&&|\|\|)/) || stream.match(/^[+\-*/%=!<>]/)) {
    return "operator";
  }
  const char = stream.next();
  if (OPENERS.includes(char)) {
    state.depth += 1;
    return "bracket";
  }
  if (CLOSERS.includes(char)) {
    state.depth = Math.max(0, state.depth - 1);
    return "bracket";
  }
  return ";:,.".includes(char) ? "punctuation" : null;
}

const language = StreamLanguage.define({
  name: "meadows",
  startState: () => ({ depth: 0 }),
  copyState: (state) => ({ depth: state.depth }),
  token,
  indent(state, textAfter, context) {
    const closing = CLOSERS.includes(textAfter.trimStart()[0] ?? "");
    return Math.max(0, state.depth - (closing ? 1 : 0)) * context.unit;
  },
  tokenTable,
  languageData: {
    commentTokens: { line: "//" },
    closeBrackets: { brackets: ["(", "[", "{", '"'] },
  },
});

const highlight = HighlightStyle.define([
  { tag: t.keyword, color: "var(--syntax-keyword)" },
  { tag: t.bool, color: "var(--syntax-number)" },
  { tag: t.string, color: "var(--syntax-string)" },
  { tag: t.number, color: "var(--syntax-number)" },
  { tag: t.comment, color: "var(--syntax-comment)", fontStyle: "italic" },
  { tag: t.operator, color: "var(--syntax-operator)" },
  { tag: t.bracket, color: "var(--syntax-punctuation)" },
  { tag: t.punctuation, color: "var(--syntax-punctuation)" },
  { tag: t.function(t.variableName), color: "var(--syntax-function)" },
  { tag: t.standard(t.variableName), color: "var(--syntax-builtin)" },
  { tag: t.variableName, color: "var(--syntax-variable)" },
]);

export function meadows() {
  return [language, syntaxHighlighting(highlight), indentUnit.of("  ")];
}
