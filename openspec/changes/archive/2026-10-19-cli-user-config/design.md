# Design

## Context

See proposal.md for motivation. Today connection keys are merged into project `.env` by `setup` (`wizard/setup.js` + `mergeEnvKey` / `mergeEnvUrl`). Load lives in `config/env.js` (`loadDotEnv`: cwd → `CLAUDE_PROJECT_DIR` → `~/.gemini/config/.env`). Serial cache already uses `~/.config/tiny-engineer/serial-port.json` but is not Windows/`XDG`/`TINY_ENGINEER_CONFIG_DIR` aware. Spec `cli-transport` currently requires project `.env` writes and uninstall `--purge-env`.

## Goals / Non-Goals

**Goals:**

- Single `userConfigDir()` helper shared by user `env` and serial-port cache.
- Dotenv format reuse (`parseLine`, `mergeEnvKey`) for user `env`.
- Setup writes connection keys only to user `env`; never edits project `.env`; hooks stay project.
- Confirm-on-diff interactive; `--yes` keeps on conflict; drop force flags.
- Warn when project `.env` overrides user connection keys (setup + doctor).
- Read BC: project `.env` still overrides when present.

**Non-Goals:**

- Migrate command or auto-strip of legacy project keys.
- `--project-env` or any “where to store?” prompt.
- `--purge-env` or any uninstall-time env stripping.
- Changing IDE hook paths or robot/firmware settings.
- New npm dependencies for path resolution.

## Decisions

### 1. Path helper + filename `env`

Export `userConfigDir()` and `userEnvPath()` from something like `config/paths.js` (or `env.js`):

- `TINY_ENGINEER_CONFIG_DIR` if set
- else win32: `join(APPDATA || homedir(), "tiny-engineer")`
- else: `join(XDG_CONFIG_HOME || join(homedir(), ".config"), "tiny-engineer")`

File: `join(userConfigDir(), "env")`.

**Why `env` not `.env`:** config dir is already the hide layer; visible name helps doctor/docs/hand-edit.

**Alt considered:** macOS Application Support — rejected to keep existing Mac cache path under `~/.config`.

### 2. Load order (first-wins with fill-if-unset)

Keep `loadDotEnvFile` semantics. Order:

1. cwd `.env`
2. `$CLAUDE_PROJECT_DIR/.env`
3. `userEnvPath()`
4. `~/.gemini/config/.env`

Process env set before load always wins. Flags stay above env in command code.

**Why project before user:** BC + intentional override without a relocate UI.

Duplicate keys within one file: **first wins** (same as fill-if-unset). `readEnvKey` used for setup conflict detection MUST match `loadDotEnvFile` (first assignment), not last.

### 3. Setup write + conflict policy

- Target path: `userEnvPath()` only for TRANSPORT/URL/PORT/TOKEN.
- Setup MUST NOT create or modify project `.env`.
- Read existing value from user `env` file (not effective/project merge) before write.
- Interactive: if different → `confirm`; equal → noop; absent → write.
- `--yes`: if different → keep + info; absent/equal as above.
- Remove `--force-url` / `--force-transport` from parser and help.
- PORT joins confirm-on-diff (stop unconditional force write).
- Token: prompt only for HTTP **and only when `args.token === undefined`**; blank skip = do not write new token; never print values. `--token` must survive Enter on an optional prompt.

**Alt considered:** `--yes` overwrite — rejected (accidental clobber).

### 3b. Setup labels global vs project (fixed destinations)

Do not ask where to store — only label:

- Connection prompts: suffix or adjacent note that value is user/global config (show `userEnvPath()` or short form like `user config`).
- Dry-run / confirm / completion lines: group or tag connection keys as global (user `env`) and hook writes as project (relative path e.g. `.cursor/hooks.json`).
- `--yes` info lines use the same labels.
- Never imply connection keys land in project `.env`.

### 3c. Project override warnings

If project `.env` defines any of `TINY_ENGINEER_TRANSPORT` / `URL` / `PORT` / `TOKEN`, setup MUST warn that those keys override user/global config for this project. Name the keys; for token report key only (never value). Still write applicable keys to user `env`. Same override reporting is required in doctor.

### 4. Storage kinds fixed

Connection → always user `env`. Hooks → always project. No prompt mixing them. Legacy project connection keys remain readable only.

### 5. Uninstall (no env purge)

Uninstall removes IDE hooks only. Remove `--purge-env` and any strip of project `.env` or user `env`. Users clear connection prefs by editing/deleting those files by hand.

### 6. Doctor

Report user config path, presence of user `env`, and whether token is set (not value). MUST list connection keys present in project `.env` that override user config (token key name only).

### 7. Tests

Drive paths via `TINY_ENGINEER_CONFIG_DIR` + temp dirs. Cover: project overrides user; user used when project absent; setup writes user not project; setup never touches project `.env`; confirm/`--yes` keep; force flags gone; `--purge-env` gone; setup/doctor override warnings; `--token` preserved; first-wins duplicates; private modes; cross-source token/URL block (omit + stderr warn). Also: interactive confirm replace and decline; blank HTTP token skip writes nothing; setup leaves an existing project `.env` byte-identical; default Unix `~/.config/tiny-engineer` path when no override env is set.

### 8. Private user env permissions

On POSIX, create user config dir with `0o700` and user `env` with `0o600` (mode on create). After writing an existing user `env`, `chmod` to `0o600` if needed so a previously world-readable file is tightened. Windows: best-effort (mode may be ignored). Do not chmod project `.env`.

### 9. Cross-source token vs project URL

Track URL/token provenance across load (process, project, user, gemini, default/flag). Before HTTP with Bearer: if URL came **only** from project `.env` and token came from user config or process env, MUST NOT send that Bearer to the project URL. **Chosen behavior:** omit the Authorization header (do not abort the request) **and** emit a one-line stderr warning that names the reason and keys involved without printing the token value. CLI `--url` / process `TINY_ENGINEER_URL` count as explicit user intent and remain allowed with user/process token. Extends the existing `tokenInherited` URL-skip idea to the user-config era. CLI help MUST note this guard.

## Risks / Trade-offs

- [Legacy project keys shadow new global writes] → Mitigation: setup + doctor MUST warn; docs say remove project TE keys to use user config; no auto-migrate.
- [Removing force flags breaks scripts] → Mitigation: document in CLI changelog; `--yes` + delete key or interactive replace.
- [Removing `--purge-env`] → Mitigation: document; hand-edit env files; uninstall still removes hooks.
- [Token in dotenv on disk] → Mitigation: user home not repo; never log raw token; private file modes; no cross-source send of user token to project URL.

## Migration Plan

- Ship read of user `env` + write-from-setup in one CLI release.
- Existing project `.env` users: behavior unchanged until they remove project keys; setup/doctor warn.
- Rollback: older CLI ignores user `env` (may miss new setups’ prefs — acceptable for downgrade).

## Open Questions

None material; review follow-ups (§8–§9, `--token` preserve, first-wins) recorded as decisions above.
