import { homedir } from "node:os";
import { join } from "node:path";

/**
 * Platform user config directory for Tiny Engineer CLI state.
 * Override with TINY_ENGINEER_CONFIG_DIR (tests / power users).
 * @returns {string}
 */
export function userConfigDir() {
  const override = process.env.TINY_ENGINEER_CONFIG_DIR?.trim();
  if (override) return override;

  if (process.platform === "win32") {
    return join(process.env.APPDATA || homedir(), "tiny-engineer");
  }

  const xdg = process.env.XDG_CONFIG_HOME?.trim();
  return join(xdg || join(homedir(), ".config"), "tiny-engineer");
}

/**
 * User-level dotenv file for connection preferences.
 * @returns {string}
 */
export function userEnvPath() {
  return join(userConfigDir(), "env");
}
