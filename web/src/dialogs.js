import { el } from "./dom.js";

export function openModal(className, content) {
  const dialog = el("dialog", { class: `modal ${className}` }, content);
  dialog.addEventListener("close", () => dialog.remove());
  dialog.addEventListener("mousedown", (event) => {
    if (event.target === dialog) {
      dialog.close();
    }
  });
  document.body.append(dialog);
  dialog.showModal();
  return dialog;
}

export function ask({ title, label, value = "", confirm = "OK" }) {
  return new Promise((resolve) => {
    const input = el("input", { type: "text", value, spellcheck: "false", autocomplete: "off", "aria-label": label });
    const form = el(
      "form",
      { method: "dialog", class: "prompt" },
      el("h2", {}, title),
      el("label", {}, label, input),
      el(
        "div",
        { class: "modal-actions" },
        el("button", { type: "button", onclick: () => dialog.close() }, "Cancel"),
        el("button", { type: "submit", class: "primary" }, confirm),
      ),
    );
    let answer = null;
    form.addEventListener("submit", () => {
      answer = input.value.trim();
    });
    const dialog = openModal("small", form);
    dialog.addEventListener("close", () => resolve(answer));
    input.select();
  });
}

export function confirmAction({ title, message, confirm }) {
  return new Promise((resolve) => {
    let accepted = false;
    const form = el(
      "form",
      { method: "dialog", class: "prompt" },
      el("h2", {}, title),
      el("p", {}, message),
      el(
        "div",
        { class: "modal-actions" },
        el("button", { type: "button", onclick: () => dialog.close() }, "Cancel"),
        el("button", { type: "submit", class: "primary", autofocus: true }, confirm),
      ),
    );
    form.addEventListener("submit", () => {
      accepted = true;
    });
    const dialog = openModal("small", form);
    dialog.addEventListener("close", () => resolve(accepted));
  });
}
