# Proposal

## Why

Connection prefs (URL, token, transport, serial port) are machine/user facts, not repo facts. Today `setup` writes them into project `.env`, which pollutes repos, puts machine paths and secrets next to shared code, and conflicts with the existing user-level serial-port cache. Prefer a platform-appropriate user config file while keeping project `.env` as a read-only override for power users and backward compatibility.

## What Changes

- Add a user config directory and dotenv file (`env`) for `TINY_ENGINEER_TRANSPORT`, `TINY_ENGINEER_URL`, `TINY_ENGINEER_PORT`, and `TINY_ENGINEER_TOKEN`.
- Resolve config: flag → process env → project `.env` → user `env` → Antigravity `~/.gemini/config/.env` → defaults.
- `setup` always writes connection keys to the **user** `env` file; IDE hooks stay project-local. Never ask whether to store global vs local. Setup MUST NOT edit project `.env` at all.
- Setup prompts and summaries MUST label connection prefs as user/global (path to user `env`) and IDE hooks as project-local (relative hook paths).
- When setup runs in a project whose `.env` sets any `TINY_ENGINEER_*` connection key, setup MUST warn that those keys override user/global config (name the keys; do not print token values). Doctor MUST report the same class of override.
- Interactive setup: confirm before replacing a stored value that differs; never echo tokens. `--yes`: keep existing on conflict (no overwrite).
- Optional HTTP auth token prompt in setup (skip / empty leaves token unset or unchanged). Interactive setup prompts for token only when `--token` was not passed (preserve flag).
- User config `env` (and its directory) written with private permissions (`0700`/`0600` on POSIX); tighten existing broad modes on write.
- Duplicate keys in dotenv: first-wins everywhere (align conflict reads with load).
- Never send a non-project Bearer token to a URL that came only from project `.env` (extend the existing process-env `tokenInherited` protection). When withheld: omit Authorization, emit a one-line stderr warning (no token value); CLI help notes the guard.
- **BREAKING** (CLI flags): remove `--force-url` and `--force-transport`.
- **BREAKING** (CLI flags): remove uninstall `--purge-env` (uninstall removes hooks only; env files are left to hand-edit).
- **BREAKING** (setup write target): new setups no longer write connection keys into project `.env` (legacy project keys still win on read if present).
- No migrate command; no `--project-env` flag; no env purge on uninstall.
- Align serial-port cache path with the same user config directory helper (Windows/`XDG`/`TINY_ENGINEER_CONFIG_DIR`).
- Update `cli-transport` requirements and docs (`integration.md`, `serial.md`, CLI help) to match.

## Capabilities

### New Capabilities

- `cli-user-config`: User-level CLI connection config location, load order, setup write rules (confirm-on-diff / `--yes` keep), override warnings, and optional token collection. Distinct from IDE hook install (project-local).

### Modified Capabilities

- `cli-transport`: Setup persistence and env resolution no longer require project `.env` as the write target; transport selection continues to honor the shared load order (including project override). Uninstall env purge requirement removed.

## Impact

- Package: `packages/tiny-engineer-cli` (`config/env.js`, new paths helper, `wizard/setup.js`, `wizard/merge.js` / uninstall flag removal, `wizard/doctor.js`, serial `port-cache.js`, tests, README/help).
- Docs: `docs/integration.md`, `docs/serial.md`, possibly `docs/hooks.md` / flash env notes if they assume project-only `.env` or `--purge-env`.
- Spec archive: merge `cli-user-config` + updated `cli-transport` on apply/archive.
- No firmware or HTTP API changes.
