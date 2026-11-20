const TOKEN_LINE = /^(\d+):(\d+)\s+(\S+)(?:\s+'(.*)')?$/;
const LOCATION = /([A-Za-z0-9_-]+\.ms):(\d+):(\d+)/g;

export function parseTokens(text) {
  return text
    .split("\n")
    .map((line) => TOKEN_LINE.exec(line))
    .filter(Boolean)
    .map((match) => ({
      line: Number(match[1]),
      column: Number(match[2]),
      type: match[3],
      text: match[4] ?? "",
    }));
}

export function parseTree(text) {
  const roots = [];
  const path = [];
  for (const line of text.split("\n")) {
    if (line.trim() === "") {
      continue;
    }
    const depth = (line.length - line.trimStart().length) / 2;
    const node = { label: line.trim(), children: [] };
    path.length = depth;
    (depth === 0 ? roots : path[depth - 1].children).push(node);
    path[depth] = node;
  }
  return roots;
}

export function splitLocations(text, fileNames) {
  const segments = [];
  let last = 0;
  for (const match of text.matchAll(LOCATION)) {
    if (!fileNames.has(match[1])) {
      continue;
    }
    if (match.index > last) {
      segments.push({ text: text.slice(last, match.index) });
    }
    segments.push({
      text: match[0],
      file: match[1],
      line: Number(match[2]),
      column: Number(match[3]),
    });
    last = match.index + match[0].length;
  }
  if (last < text.length) {
    segments.push({ text: text.slice(last) });
  }
  return segments;
}

export function parseBytecode(text) {
  const functions = [];
  for (const line of text.split("\n")) {
    const header = /^func (\S+)\s+params (\d+)\s+slots (\d+)/.exec(line);
    if (header) {
      functions.push({
        name: header[1],
        params: Number(header[2]),
        slots: Number(header[3]),
        rows: [],
      });
    } else if (functions.length > 0) {
      const match = /^\s*(\d+)\s+(\w+)\s*(.*?)(?:\s*; line (\d+))?$/.exec(line);
      if (match) {
        functions.at(-1).rows.push({
          index: Number(match[1]),
          op: match[2],
          operands: match[3],
          line: match[4] ? Number(match[4]) : null,
        });
      }
    }
  }
  return functions;
}
