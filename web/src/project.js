export const ENTRY_FILE = "main.ms";
const FILE_NAME = /^[A-Za-z0-9_-]+\.ms$/;
const STORAGE_KEY = "meadows.project";
const MAX_FILES = 16;

export function isValidFileName(name) {
  return FILE_NAME.test(name);
}

export function createProject(files, active = ENTRY_FILE) {
  return { files: files.map(({ name, text }) => ({ name, text })), active };
}

export function addFile(project, name) {
  if (!isValidFileName(name)) {
    throw new Error("File names may contain letters, digits, '_' and '-' and must end in .ms");
  }
  if (project.files.some((file) => file.name === name)) {
    throw new Error(`${name} already exists`);
  }
  if (project.files.length >= MAX_FILES) {
    throw new Error(`A project can hold at most ${MAX_FILES} files`);
  }
  project.files.push({ name, text: "" });
  project.active = name;
}

export function renameFile(project, from, to) {
  if (from === ENTRY_FILE) {
    throw new Error(`${ENTRY_FILE} is the entry point and cannot be renamed`);
  }
  if (from === to) {
    return;
  }
  if (!isValidFileName(to)) {
    throw new Error("File names may contain letters, digits, '_' and '-' and must end in .ms");
  }
  if (project.files.some((file) => file.name === to)) {
    throw new Error(`${to} already exists`);
  }
  const file = project.files.find((candidate) => candidate.name === from);
  file.name = to;
  if (project.active === from) {
    project.active = to;
  }
}

export function upsertFile(project, name, text) {
  const existing = project.files.find((file) => file.name === name);
  if (existing) {
    existing.text = text;
    return;
  }
  addFile(project, name);
  project.files.find((file) => file.name === name).text = text;
}

export function removeFile(project, name) {
  if (name === ENTRY_FILE) {
    throw new Error(`${ENTRY_FILE} is the entry point and cannot be removed`);
  }
  project.files = project.files.filter((file) => file.name !== name);
  if (project.active === name) {
    project.active = ENTRY_FILE;
  }
}

export function activeFile(project) {
  return project.files.find((file) => file.name === project.active);
}

function isProject(value) {
  return (
    value &&
    Array.isArray(value.files) &&
    value.files.length > 0 &&
    value.files.length <= MAX_FILES &&
    value.files.every(
      (file) => file && isValidFileName(file.name) && typeof file.text === "string",
    ) &&
    value.files.some((file) => file.name === ENTRY_FILE)
  );
}

function normalise(value) {
  const active = value.files.some((file) => file.name === value.active)
    ? value.active
    : ENTRY_FILE;
  return createProject(value.files, active);
}

export function saveProject(project) {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(project));
  } catch {
    // Storage can be unavailable or full; the project is still usable in memory.
  }
}

export function loadSavedProject() {
  try {
    const value = JSON.parse(localStorage.getItem(STORAGE_KEY));
    return isProject(value) ? normalise(value) : null;
  } catch {
    return null;
  }
}

function toBase64Url(bytes) {
  let binary = "";
  for (const byte of bytes) {
    binary += String.fromCharCode(byte);
  }
  return btoa(binary).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");
}

function fromBase64Url(text) {
  const padded = text.replace(/-/g, "+").replace(/_/g, "/");
  const binary = atob(padded + "=".repeat((4 - (padded.length % 4)) % 4));
  return Uint8Array.from(binary, (char) => char.charCodeAt(0));
}

async function pipe(bytes, stream) {
  const result = new Response(new Blob([bytes]).stream().pipeThrough(stream));
  return new Uint8Array(await result.arrayBuffer());
}

export async function encodeProject(project) {
  const json = new TextEncoder().encode(JSON.stringify(project));
  return toBase64Url(await pipe(json, new CompressionStream("deflate-raw")));
}

export async function decodeProject(text) {
  try {
    const json = await pipe(fromBase64Url(text), new DecompressionStream("deflate-raw"));
    const value = JSON.parse(new TextDecoder().decode(json));
    return isProject(value) ? normalise(value) : null;
  } catch {
    return null;
  }
}
