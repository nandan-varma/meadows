// Meadows language support for CodeMirror 6: a StreamLanguage tokenizer
// (the CodeMirror-5-style token API, still supported directly by
// @codemirror/language) plus a matching highlight style. Kept in its own
// module so app.js doesn't need to know how Meadows is lexed.
//
// Import specifiers are unpinned major versions (`@6`), not exact patches —
// pinning exact, independently-chosen patch versions across these packages
// previously resolved to *different* internal copies of @codemirror/state
// on esm.sh, which silently dropped every extension (no gutters, no
// highlighting, no error). Letting esm.sh pick one mutually-compatible set
// avoids that; verified in a real browser before landing this.
import { StreamLanguage, HighlightStyle, syntaxHighlighting } from "https://esm.sh/@codemirror/language@6";
import { tags as t } from "https://esm.sh/@lezer/highlight@1";

// Mirrors src/lexer/Token.h exactly — keep in sync if the keyword list changes.
const KEYWORDS = new Set([
  "let", "func", "if", "else", "for", "while", "return", "in", "range",
  "break", "continue", "import",
]);
const BOOLEANS = new Set(["true", "false"]);
// Not reserved words — ordinary identifiers the compiler treats specially.
const BUILTINS = new Set(["print", "len", "str", "push"]);

// Explicit mapping from this tokenizer's own token names to highlight tags —
// avoids relying on StreamLanguage's legacy CodeMirror-5 name inference.
const TOKEN_TABLE = {
  kw: t.keyword,
  bool: t.bool,
  str: t.string,
  num: t.number,
  cmt: t.comment,
  op: t.operator,
  brk: t.bracket,
  punct: t.punctuation,
  fn: t.function(t.variableName),
  builtin: t.standard(t.variableName),
  var: t.variableName,
};

function token(stream) {
  if (stream.eatSpace()) return null;

  if (stream.match("#") || stream.match("//")) {
    stream.skipToEnd();
    return "cmt";
  }

  if (stream.match(/^"(?:[^"\\]|\\.)*"?/)) return "str";
  if (stream.match(/^\d+\.\d+/) || stream.match(/^\d+/)) return "num";

  if (stream.match(/^[A-Za-z_][A-Za-z0-9_]*/)) {
    const word = stream.current();
    if (KEYWORDS.has(word)) return "kw";
    if (BOOLEANS.has(word)) return "bool";
    if (BUILTINS.has(word)) return "builtin";
    return stream.peek() === "(" ? "fn" : "var";
  }

  if (stream.match(/^(==|!=|<=|>=|&&|\|\|)/)) return "op";
  if (stream.match(/^[+\-*/%=!<>]/)) return "op";
  if (stream.match(/^[{}()\[\]]/)) return "brk";
  if (stream.match(/^[;:,.]/)) return "punct";

  stream.next();
  return null;
}

export const meadowsLanguage = StreamLanguage.define({
  name: "meadows",
  token,
  tokenTable: TOKEN_TABLE,
  languageData: {
    commentTokens: { line: "//" },
  },
});

// Matches the playground's existing dark theme (see styles.css --accent etc.)
// rather than pulling in a separate theme package for a handful of colors.
export const meadowsHighlight = syntaxHighlighting(
  HighlightStyle.define([
    { tag: t.keyword, color: "#c792ea" },
    { tag: t.bool, color: "#f78c6c" },
    { tag: t.string, color: "#c3e88d" },
    { tag: t.number, color: "#f78c6c" },
    { tag: t.comment, color: "#697098", fontStyle: "italic" },
    { tag: t.operator, color: "#89ddff" },
    { tag: t.bracket, color: "#8b93a7" },
    { tag: t.punctuation, color: "#8b93a7" },
    { tag: t.function(t.variableName), color: "#82aaff" },
    { tag: t.standard(t.variableName), color: "#6ee7b7" },
    { tag: t.variableName, color: "#e2e4e9" },
  ])
);

export function meadowsSetup() {
  return [meadowsLanguage, meadowsHighlight];
}
