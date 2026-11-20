const FUNCTION = /^func\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)/;
const VARIABLE = /^let\s+([A-Za-z_][A-Za-z0-9_]*)/;
const DECLARATION = /\b(?:let\s+|for\s*\(\s*)([A-Za-z_][A-Za-z0-9_]*)/g;
const PARAMETERS = /\bfunc\s+[A-Za-z_][A-Za-z0-9_]*\s*\(([^)]*)\)/g;

function stripComment(line) {
  let quoted = false;
  for (let i = 0; i < line.length; i++) {
    const char = line[i];
    if (quoted) {
      if (char === "\\") {
        i++;
      } else if (char === '"') {
        quoted = false;
      }
    } else if (char === '"') {
      quoted = true;
    } else if (char === "#" || (char === "/" && line[i + 1] === "/")) {
      return line.slice(0, i);
    }
  }
  return line;
}

function splitParameters(text) {
  return text
    .split(",")
    .map((part) => part.trim())
    .filter(Boolean);
}

export function findSymbols(files) {
  const symbols = [];
  for (const file of files) {
    file.text.split("\n").forEach((raw, index) => {
      const line = stripComment(raw);
      const func = FUNCTION.exec(line);
      if (func) {
        const parameters = splitParameters(func[2]);
        symbols.push({
          kind: "function",
          name: func[1],
          parameters,
          signature: `${func[1]}(${parameters.join(", ")})`,
          file: file.name,
          line: index + 1,
          column: raw.indexOf(func[1]) + 1,
        });
        return;
      }
      const variable = VARIABLE.exec(line);
      if (variable) {
        symbols.push({
          kind: "variable",
          name: variable[1],
          parameters: [],
          signature: variable[1],
          file: file.name,
          line: index + 1,
          column: raw.indexOf(variable[1]) + 1,
        });
      }
    });
  }
  return symbols;
}

export function findLocals(text) {
  const names = new Set();
  const code = text.split("\n").map(stripComment).join("\n");
  for (const match of code.matchAll(DECLARATION)) {
    names.add(match[1]);
  }
  for (const match of code.matchAll(PARAMETERS)) {
    splitParameters(match[1]).forEach((name) => names.add(name));
  }
  return [...names];
}
