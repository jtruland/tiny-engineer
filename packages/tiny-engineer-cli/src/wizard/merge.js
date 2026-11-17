import {
  chmodSync,
  copyFileSync,
  existsSync,
  mkdirSync,
  readFileSync,
  writeFileSync,
} from "node:fs";
import { dirname } from "node:path";

import { parseDotEnvLine } from "../config/env.js";

const PRIVATE_DIR_MODE = 0o700;
const PRIVATE_FILE_MODE = 0o600;

/**
 * Ensure parent dir exists; optionally with private POSIX modes.
 * @param {string} envPath
 * @param {boolean} privateMode
 */
function ensureEnvParent(envPath, privateMode) {
  const dir = dirname(envPath);
  if (privateMode) {
    mkdirSync(dir, { recursive: true, mode: PRIVATE_DIR_MODE });
    try {
      chmodSync(dir, PRIVATE_DIR_MODE);
    } catch {
      // Windows / unsupported chmod: best-effort
    }
  } else {
    mkdirSync(dir, { recursive: true });
  }
}

/**
 * Write dotenv contents; optionally private file mode + tighten existing.
 * @param {string} envPath
 * @param {string} contents
 * @param {boolean} privateMode
 */
function writeEnvFile(envPath, contents, privateMode) {
  if (privateMode) {
    writeFileSync(envPath, contents, { encoding: "utf8", mode: PRIVATE_FILE_MODE });
    try {
      chmodSync(envPath, PRIVATE_FILE_MODE);
    } catch {
      // Windows / unsupported chmod: best-effort
    }
  } else {
    writeFileSync(envPath, contents, "utf8");
  }
}

/**
 * @param {string} path
 * @returns {object | null}
 */
export function readJsonFile(path) {
  if (!existsSync(path)) return null;
  try {
    const raw = readFileSync(path, "utf8");
    const parsed = JSON.parse(raw);
    return parsed && typeof parsed === "object" ? parsed : null;
  } catch {
    return null;
  }
}

/**
 * Write JSON; create parent dirs; backup once to `path.bak` if file exists and bak does not.
 * @param {string} path
 * @param {object} data
 */
export function writeJsonFile(path, data) {
  mkdirSync(dirname(path), { recursive: true });
  if (existsSync(path) && !existsSync(`${path}.bak`)) {
    copyFileSync(path, `${path}.bak`);
  }
  writeFileSync(path, `${JSON.stringify(data, null, 2)}\n`, "utf8");
}

/**
 * Read a single key from a dotenv file (first matching line wins).
 * @param {string} envPath
 * @param {string} key
 * @returns {string | null} trimmed value, or null if absent/empty
 */
export function readEnvKey(envPath, key) {
  if (!existsSync(envPath)) return null;
  let raw;
  try {
    raw = readFileSync(envPath, "utf8");
  } catch {
    return null;
  }
  for (const line of raw.split(/\r?\n/)) {
    const parsed = parseDotEnvLine(line);
    if (!parsed) continue;
    const [k, value] = parsed;
    if (k === key) return value.trim() || null;
  }
  return null;
}

/**
 * Merge a Tiny Engineer env key into a dotenv file.
 * @param {string} envPath
 * @param {string} key
 * @param {string} value
 * @param {{ force?: boolean, private?: boolean }} [opts]
 * @returns {"created" | "updated" | "skipped" | "unchanged"}
 */
export function mergeEnvKey(envPath, key, value, { force = false, private: privateMode = false } = {}) {
  const existing = readEnvKey(envPath, key);
  if (existing !== null && existing === value) {
    if (privateMode && existsSync(envPath)) {
      try {
        chmodSync(envPath, PRIVATE_FILE_MODE);
      } catch {
        // best-effort
      }
    }
    return "unchanged";
  }

  const re = new RegExp(`^\\s*${key}\\s*=`);
  ensureEnvParent(envPath, privateMode);
  let raw = "";
  if (existsSync(envPath)) {
    raw = readFileSync(envPath, "utf8");
  }

  const lines = raw ? raw.split(/\r?\n/) : [];
  let found = false;
  const next = lines.map((line) => {
    if (!re.test(line)) return line;
    found = true;
    if (!force) return line;
    return `${key}=${value}`;
  });

  if (found && !force) return "skipped";

  if (!found) {
    if (next.length && next[next.length - 1] !== "") next.push("");
    next.push(`${key}=${value}`);
    writeEnvFile(envPath, `${next.join("\n").replace(/\n+$/, "\n")}`, privateMode);
    return raw ? "updated" : "created";
  }

  writeEnvFile(envPath, `${next.join("\n").replace(/\n+$/, "\n")}`, privateMode);
  return "updated";
}

/**
 * Merge TINY_ENGINEER_URL into a dotenv file.
 * @param {string} envPath
 * @param {string} url
 * @param {{ force?: boolean }} [opts]
 * @returns {"created" | "updated" | "skipped" | "unchanged"}
 */
export function mergeEnvUrl(envPath, url, { force = false } = {}) {
  return mergeEnvKey(envPath, "TINY_ENGINEER_URL", url, { force });
}
