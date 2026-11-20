const WORKER_URL = new URL("./worker.js", import.meta.url);
const OUTPUT_LIMIT = 1_000_000;

function spawn() {
  return new Worker(WORKER_URL, { type: "module" });
}

export class Analyzer {
  #worker = spawn();
  #sequence = 0;
  #pending = new Map();
  #ready;

  constructor() {
    this.#ready = new Promise((resolve) => {
      this.#worker.addEventListener("message", ({ data }) => {
        if (data.type === "ready") {
          resolve(data.version);
        } else if (data.type === "analysis") {
          this.#settle(data.id, (entry) => entry.resolve(data.report));
        } else if (data.type === "failure") {
          this.#settle(data.id, (entry) => entry.reject(new Error(data.message)));
        }
      });
    });
  }

  #settle(id, action) {
    const entry = this.#pending.get(id);
    this.#pending.delete(id);
    if (entry) {
      action(entry);
    }
  }

  get version() {
    return this.#ready;
  }

  analyze(project, entry) {
    const id = ++this.#sequence;
    for (const [staleId, stale] of this.#pending) {
      stale.resolve(null);
      this.#pending.delete(staleId);
    }
    return new Promise((resolve, reject) => {
      this.#pending.set(id, { resolve, reject });
      this.#worker.postMessage({ type: "analyze", id, files: project.files, entry });
    });
  }
}

export class Runner {
  #idle = spawn();
  #active = null;

  get running() {
    return this.#active !== null;
  }

  run(project, entry, { onOutput, onFinish }) {
    const worker = this.#idle;
    this.#idle = spawn();
    const started = performance.now();
    let received = 0;
    let limited = false;

    const finish = (result) => {
      worker.terminate();
      this.#active = null;
      onFinish({ ...result, elapsed: result.elapsed ?? performance.now() - started });
    };

    worker.addEventListener("message", ({ data }) => {
      if (limited) {
        return;
      }
      if (data.type === "output") {
        const room = OUTPUT_LIMIT - received;
        received += data.text.length;
        onOutput(data.text.slice(0, Math.max(room, 0)));
        if (received > OUTPUT_LIMIT) {
          limited = true;
          finish({ status: "limit" });
        }
      } else if (data.type === "done") {
        finish({ status: "done", report: data.report });
      } else if (data.type === "failure") {
        finish({ status: "failure", message: data.message });
      }
    });
    worker.addEventListener("error", (event) => {
      finish({ status: "failure", message: event.message || "The worker crashed" });
    });
    worker.postMessage({ type: "run", files: project.files, entry });

    this.#active = {
      stop: () => finish({ status: "stopped" }),
    };
  }

  stop() {
    this.#active?.stop();
  }
}

export { OUTPUT_LIMIT };
