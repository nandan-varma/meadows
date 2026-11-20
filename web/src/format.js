const OPENERS = "([{";
const CLOSERS = ")]}";

function scan(line) {
  let opened = 0;
  let closed = 0;
  let leadingClosers = 0;
  let leading = true;
  let quoted = false;
  for (let i = 0; i < line.length; i++) {
    const char = line[i];
    if (quoted) {
      if (char === "\\") {
        i++;
      } else if (char === '"') {
        quoted = false;
      }
      continue;
    }
    if (char === '"') {
      quoted = true;
      leading = false;
    } else if (char === "#" || (char === "/" && line[i + 1] === "/")) {
      break;
    } else if (OPENERS.includes(char)) {
      opened++;
      leading = false;
    } else if (CLOSERS.includes(char)) {
      closed++;
      if (leading) {
        leadingClosers++;
      }
    } else if (char !== " " && char !== "\t") {
      leading = false;
    }
  }
  return { opened, closed, leadingClosers };
}

export function formatSource(text) {
  let depth = 0;
  let previousBlank = true;
  const lines = [];
  for (const raw of text.split("\n")) {
    const line = raw.trim();
    if (line === "") {
      if (!previousBlank) {
        lines.push("");
      }
      previousBlank = true;
      continue;
    }
    previousBlank = false;
    const { opened, closed, leadingClosers } = scan(line);
    lines.push("  ".repeat(Math.max(0, depth - leadingClosers)) + line);
    depth = Math.max(0, depth + opened - closed);
  }
  while (lines.at(-1) === "") {
    lines.pop();
  }
  return lines.length === 0 ? "" : `${lines.join("\n")}\n`;
}
