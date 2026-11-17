import { readFileSync } from "node:fs";
import { homedir } from "node:os";
import { join, resolve } from "node:path";

import { userEnvPath } from "./paths.js";

export const TOKEN_KEY = "TINY_ENGINEER_TOKEN";
export const URL_KEY = "TINY_ENGINEER_URL";
export const TRANSPORT_KEY = "TINY_ENGINEER_TRANSPORT";
export const PORT_KEY = "TINY_ENGINEER_PORT";
/** Connection keys owned by user config (and readable as project overrides). */
export const CONNECTION_KEYS = [TRANSPORT_KEY, URL_KEY, PORT_KEY, TOKEN_KEY];
export const DEFAULT_URL = "http://tiny-engineer.local";
export const DEFAULT_TRANSPORT = "http";

/**
 * @typedef {"process" | "project" | "user" | "gemini"} ConnectionSource
 */

/** @type {{ url: ConnectionSource | null, token: ConnectionSource | null }} */
let connectionProvenance = { url: null, token: null };

/**
 * Where URL / token last came from during load (for cross-source auth guard).
 * @returns {{ url: ConnectionSource | null, token: ConnectionSource | null }}
 */
export function getConnectionProvenance() {
  return { url: connectionProvenance.url, token: connectionProvenance.token };
}

/**
 * @param {string} key
 * @param {ConnectionSource} source
 */
function noteConnectionProvenance(key, source) {
  if (key === URL_KEY && connectionProvenance.url === null) {
    connectionProvenance.url = source;
  }
  if (key === TOKEN_KEY && connectionProvenance.token === null) {
    connectionProvenance.token = source;
  }
}

/**
 * @param {string | undefined | null} value
 * @returns {"http" | "serial" | null}
 */
export function normalizeTransport(value) {
  if (value === undefined || value === null) return null;
  const trimmed = String(value).trim().toLowerCase();
  if (trimmed === "http" || trimmed === "serial") return trimmed;
  return null;
}

/**
 * Parse one dotenv line into [key, value] or null.
 * @param {string} line
 * @returns {[string, string] | null}
 */
export function parseDotEnvLine(line) {
  const trimmed = line.trim();
  if (!trimmed || trimmed.startsWith("#")) return null;

  const eq = trimmed.indexOf("=");
  if (eq <= 0) return null;

  const key = trimmed.slice(0, eq).trim();
  if (!/^[A-Za-z_][A-Za-z0-9_]*$/.test(key)) return null;

  let value = trimmed.slice(eq + 1).trim();
  if (
    (value.startsWith('"') && value.endsWith('"')) ||
    (value.startsWith("'") && value.endsWith("'"))
  ) {
    value = value.slice(1, -1);
  }
  return [key, value];
}

/**
 * Load one `.env` file into process.env for keys not already set.
 * A token already in the process env must not be sent to a URL from project
 * `.env`, so project files skip `TINY_ENGINEER_URL` when a token was inherited.
 * Missing file / read errors are ignored.
 * @param {string} filePath
 * @param {ConnectionSource} [source]
 */
export function loadDotEnvFile(filePath, source = "project") {
  let raw;
  try {
    raw = readFileSync(filePath, "utf8");
  } catch {
    return;
  }

  const tokenInherited = Boolean(process.env[TOKEN_KEY]?.trim());

  for (const line of raw.split(/\r?\n/)) {
    const parsed = parseDotEnvLine(line);
    if (!parsed) continue;
    const [key, value] = parsed;
    // Only skip project URL when process already holds a token (not user/gemini URL).
    if (key === URL_KEY && tokenInherited && source === "project") continue;
    if (process.env[key] === undefined) {
      process.env[key] = value;
      noteConnectionProvenance(key, source);
    }
  }
}

/**
 * Connection keys present in a dotenv file (values not returned for TOKEN safety at call sites).
 * @param {string} filePath
 * @returns {string[]}
 */
export function listConnectionKeysInFile(filePath) {
  let raw;
  try {
    raw = readFileSync(filePath, "utf8");
  } catch {
    return [];
  }
  /** @type {Set<string>} */
  const found = new Set();
  for (const line of raw.split(/\r?\n/)) {
    const parsed = parseDotEnvLine(line);
    if (!parsed) continue;
    const [key, value] = parsed;
    if (CONNECTION_KEYS.includes(key) && value.trim()) found.add(key);
  }
  return CONNECTION_KEYS.filter((k) => found.has(k));
}

/**
 * Load env from cwd, Claude project dir, user config, then Antigravity global.
 * Earlier sources win (fill-if-unset).
 * @param {string} [cwd]
 */
export function loadDotEnv(cwd = process.cwd()) {
  connectionProvenance = { url: null, token: null };
  if (process.env[URL_KEY]?.trim()) connectionProvenance.url = "process";
  if (process.env[TOKEN_KEY]?.trim()) connectionProvenance.token = "process";

  const loaded = new Set();

  /**
   * @param {string} dir
   */
  function loadDir(dir) {
    const abs = resolve(dir);
    if (loaded.has(abs)) return;
    loaded.add(abs);
    loadDotEnvFile(join(abs, ".env"), "project");
  }

  loadDir(cwd);

  const claudeRoot = process.env.CLAUDE_PROJECT_DIR?.trim();
  if (claudeRoot) loadDir(claudeRoot);

  loadDotEnvFile(userEnvPath(), "user");

  loadDotEnvFile(join(homedir(), ".gemini", "config", ".env"), "gemini");
}

/**
 * @returns {string | null} trimmed TINY_ENGINEER_TOKEN, or null if unset/empty
 */
export function getToken() {
  const value = process.env[TOKEN_KEY];
  if (value === undefined) return null;
  const trimmed = value.trim();
  return trimmed ? trimmed : null;
}

/**
 * Token for HTTP Authorization. Blocks user/process token against project-only URL.
 * @param {{ urlOverride?: boolean }} [opts] true when CLI `--url` supplies the base
 * @returns {string | null}
 */
export function getHttpAuthToken(opts = {}) {
  const token = getToken();
  if (!token) return null;
  if (opts.urlOverride) return token;

  const { url: urlSrc, token: tokenSrc } = connectionProvenance;
  // process URL is explicit intent; unset provenance means default URL
  if (urlSrc === "process" || urlSrc === null) return token;
  if (urlSrc === "project" && (tokenSrc === "user" || tokenSrc === "process")) {
    process.stderr.write(
      `warning: withholding Authorization — project ${URL_KEY} with ${tokenSrc} ${TOKEN_KEY}\n`,
    );
    return null;
  }
  return token;
}

/**
 * @returns {string} TINY_ENGINEER_URL without trailing slashes, or the default
 */
export function getBaseUrl() {
  const value = process.env[URL_KEY];
  if (value === undefined) return DEFAULT_URL;
  const trimmed = value.trim().replace(/\/+$/, "");
  return trimmed || DEFAULT_URL;
}

/**
 * Resolve host transport: flag > TINY_ENGINEER_TRANSPORT > default http.
 * @param {string | undefined} [flag] value from `--transport`
 * @returns {{ transport: "http" | "serial", error?: undefined } | { transport?: undefined, error: string }}
 */
export function resolveTransport(flag) {
  if (flag !== undefined) {
    const normalized = normalizeTransport(flag);
    if (!normalized) {
      return { error: `invalid --transport ${flag} (use http or serial)` };
    }
    return { transport: normalized };
  }

  const envValue = process.env[TRANSPORT_KEY];
  if (envValue !== undefined && envValue.trim() !== "") {
    const normalized = normalizeTransport(envValue);
    if (!normalized) {
      return { error: `invalid TINY_ENGINEER_TRANSPORT=${envValue} (use http or serial)` };
    }
    return { transport: normalized };
  }

  return { transport: DEFAULT_TRANSPORT };
}

/**
 * @returns {string | null} trimmed TINY_ENGINEER_PORT, or null if unset/empty
 */
export function getSerialPort() {
  const value = process.env[PORT_KEY];
  if (value === undefined) return null;
  const trimmed = value.trim();
  return trimmed ? trimmed : null;
}
