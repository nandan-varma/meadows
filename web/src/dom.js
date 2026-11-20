const SVG_NAMESPACE = "http://www.w3.org/2000/svg";

export function el(tag, attributes = {}, ...children) {
  const node = document.createElement(tag);
  for (const [name, value] of Object.entries(attributes)) {
    if (value === false || value === null || value === undefined) {
      continue;
    }
    if (name === "class") {
      node.className = value;
    } else if (name.startsWith("on")) {
      node.addEventListener(name.slice(2), value);
    } else {
      node.setAttribute(name, value === true ? "" : value);
    }
  }
  node.append(...children.flat().filter((child) => child !== null && child !== undefined && child !== false));
  return node;
}

export function icon(name) {
  const svg = document.createElementNS(SVG_NAMESPACE, "svg");
  svg.setAttribute("class", "icon");
  svg.setAttribute("aria-hidden", "true");
  const use = document.createElementNS(SVG_NAMESPACE, "use");
  use.setAttribute("href", `#i-${name}`);
  svg.append(use);
  return svg;
}

export function kbd(shortcut) {
  const keys = shortcut.split("+").map((key) => (key === "Mod" ? (isMac() ? "⌘" : "Ctrl") : key));
  return el("span", { class: "keys" }, ...keys.map((key) => el("kbd", {}, key)));
}

export function isMac() {
  return /Mac|iPhone|iPad/.test(navigator.platform);
}
