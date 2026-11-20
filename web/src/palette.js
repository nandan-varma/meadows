import { el, kbd } from "./dom.js";
import { openModal } from "./dialogs.js";

const MAX_RESULTS = 60;

export function score(query, text) {
  if (query === "") {
    return 0;
  }
  const haystack = text.toLowerCase();
  let position = 0;
  let total = 0;
  let streak = 0;
  for (const char of query.toLowerCase()) {
    const found = haystack.indexOf(char, position);
    if (found < 0) {
      return null;
    }
    streak = found === position ? streak + 1 : 0;
    const boundary = found === 0 || /[\s._\-/]/.test(haystack[found - 1]);
    total += 1 + streak * 2 + (boundary ? 3 : 0) - Math.min(found - position, 5) * 0.2;
    position = found + 1;
  }
  return total - text.length * 0.01;
}

export function rank(items, query) {
  const needle = query.trim();
  return items
    .map((item) => ({ item, value: score(needle, `${item.title} ${item.group}`) }))
    .filter((entry) => entry.value !== null)
    .sort((a, b) => b.value - a.value)
    .slice(0, MAX_RESULTS)
    .map((entry) => entry.item);
}

export function openPalette(items, placeholder = "Search commands, files and symbols") {
  const input = el("input", {
    type: "text",
    placeholder,
    spellcheck: "false",
    autocomplete: "off",
    role: "combobox",
    "aria-expanded": "true",
    "aria-controls": "palette-results",
    "aria-label": placeholder,
  });
  const list = el("ul", { id: "palette-results", role: "listbox" });
  let results = [];
  let selected = 0;
  let dialog;

  const choose = (item) => {
    dialog.close();
    queueMicrotask(item.run);
  };

  const paint = () => {
    list.replaceChildren(
      ...results.map((item, index) =>
        el(
          "li",
          {
            role: "option",
            "aria-selected": String(index === selected),
            class: item.group.toLowerCase(),
            onclick: () => choose(item),
            onmousemove: () => {
              if (selected !== index) {
                selected = index;
                paint();
              }
            },
          },
          el("span", { class: "title" }, item.title),
          item.detail ? el("span", { class: "detail" }, item.detail) : null,
          el("span", { class: "group" }, item.group),
          item.keys ? kbd(item.keys) : null,
        ),
      ),
    );
    if (results.length === 0) {
      list.append(el("li", { class: "empty" }, "Nothing matches"));
    }
    list.children[selected]?.scrollIntoView({ block: "nearest" });
  };

  const update = () => {
    results = rank(items, input.value);
    selected = 0;
    paint();
  };

  input.addEventListener("input", update);
  input.addEventListener("keydown", (event) => {
    if (event.key === "ArrowDown" || event.key === "ArrowUp") {
      event.preventDefault();
      const step = event.key === "ArrowDown" ? 1 : -1;
      selected = (selected + step + results.length) % Math.max(results.length, 1);
      paint();
    } else if (event.key === "Enter" && results[selected]) {
      event.preventDefault();
      choose(results[selected]);
    }
  });

  dialog = openModal("palette", [input, list]);
  update();
}
