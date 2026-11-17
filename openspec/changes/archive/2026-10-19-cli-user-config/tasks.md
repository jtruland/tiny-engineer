# Tasks

## 1. User config paths and load order

- [x] 1.1 Add `userConfigDir()` / `userEnvPath()` (and wire `TINY_ENGINEER_CONFIG_DIR`, Windows `APPDATA`, `XDG_CONFIG_HOME`) and point serial port cache at the same directory; verify with unit tests using a temp `TINY_ENGINEER_CONFIG_DIR`
- [x] 1.2 Extend `loadDotEnv` to load user `env` after project / Claude project `.env` and before `~/.gemini/config/.env`; verify project overrides user and user fills when project key absent

## 2. Setup writes user env with conflict rules

- [x] 2.1 Change setup to merge `TRANSPORT` / `URL` / `PORT` only into user `env` and never create or modify project `.env`; remove `--force-url` and `--force-transport`; verify existing setup tests updated and pass
- [x] 2.2 Implement interactive confirm-on-diff and `--yes` keep-on-conflict for those keys (PORT included); verify tests cover confirm replace, decline, and `--yes` keep
- [x] 2.3 Add optional HTTP-only token prompt/merge into user `env` under the same conflict rules; verify serial setup skips token and blank HTTP skip does not write token; never print token values
- [x] 2.4 Label setup prompts/dry-run/summary so connection keys show as user/global and hooks as project; verify test or asserted output strings
- [x] 2.5 Warn during setup when project `.env` overrides connection keys; verify warning names keys and never prints token value

## 3. Uninstall, doctor, docs

- [x] 3.1 Remove uninstall `--purge-env` and related strip/merge helpers usage; verify uninstall tests no longer offer or exercise purge-env
- [x] 3.2 Update doctor to report user config path / user `env` presence, token set/unset, and MUST list project `.env` override keys without secrets; verify doctor output in tests or golden snippet
- [x] 3.3 Update `docs/integration.md`, `docs/serial.md`, and CLI help/README for load order, user `env` path, dropped force flags, removed `--purge-env`, and override warnings; verify docs match implemented flags and paths

## 4. Integration check

- [x] 4.1 Run `npm test --prefix packages/tiny-engineer-cli` and fix failures until green

## 5. Review follow-ups

- [x] 5.1 Preserve `--token` in interactive setup (prompt only if undefined); verify Enter does not clear flag
- [x] 5.2 Align `readEnvKey` to first-wins; verify duplicate-key conflict matches load
- [x] 5.3 Private perms for user config dir/env (`0700`/`0600` + chmod on write); verify on POSIX
- [x] 5.4 Block user/process token against project-only URL; verify with unit test
- [x] 5.5 Re-run `npm test --prefix packages/tiny-engineer-cli` until green

## 6. Verify follow-ups (warn + coverage)

- [x] 6.1 When cross-source auth withholds Bearer, emit one-line stderr warning (no token value); verify with unit test
- [x] 6.2 CLI help/README one-line caveat for the cross-source token/URL guard
- [x] 6.3 Add tests: interactive confirm replace and decline; blank HTTP token skip writes no token; setup leaves existing project `.env` byte-identical; default Unix `~/.config/tiny-engineer` path without override env
- [x] 6.4 Re-run `npm test --prefix packages/tiny-engineer-cli` until green

## Workflow follow-up

- Archive the change after review (`/opsx-archive` or equivalent) so `cli-user-config` lands in main specs and `cli-transport` deltas merge.
