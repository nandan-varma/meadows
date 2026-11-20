const STORAGE_KEY = "meadows.settings";

export const THEMES = ["system", "light", "dark", "midnight"];

export const DEFAULTS = {
  theme: "system",
  fontSize: 14,
  vim: false,
  wrap: false,
  autorun: false,
  stacked: false,
  split: 0.55,
};

const RULES = {
  theme: (value) => THEMES.includes(value),
  fontSize: (value) => Number.isInteger(value) && value >= 11 && value <= 24,
  vim: (value) => typeof value === "boolean",
  wrap: (value) => typeof value === "boolean",
  autorun: (value) => typeof value === "boolean",
  stacked: (value) => typeof value === "boolean",
  split: (value) => typeof value === "number" && value >= 0.2 && value <= 0.8,
};

export function normaliseSettings(value) {
  const settings = { ...DEFAULTS };
  for (const [key, valid] of Object.entries(RULES)) {
    if (value && valid(value[key])) {
      settings[key] = value[key];
    }
  }
  return settings;
}

function load() {
  try {
    return normaliseSettings(JSON.parse(localStorage.getItem(STORAGE_KEY)));
  } catch {
    return { ...DEFAULTS };
  }
}

function persist(values) {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(values));
  } catch {
    return;
  }
}

export class Settings {
  #values = load();
  #listeners = new Set();

  get(key) {
    return this.#values[key];
  }

  set(key, value) {
    if (!RULES[key](value) || this.#values[key] === value) {
      return;
    }
    this.#values[key] = value;
    persist(this.#values);
    this.#listeners.forEach((listener) => listener(key, value));
  }

  subscribe(listener) {
    this.#listeners.add(listener);
    listener(null, null);
  }
}
