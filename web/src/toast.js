import { el } from "./dom.js";

const LIFETIME_MS = 3200;

export function toast(message, kind = "info") {
  const host = document.getElementById("toasts");
  const node = el("div", { class: `toast ${kind}`, role: "status" }, message);
  host.append(node);
  setTimeout(() => {
    node.classList.add("leaving");
    node.addEventListener("animationend", () => node.remove(), { once: true });
  }, LIFETIME_MS);
}
