# Design

## Context

See proposal.md — Why. Current `main` parks PWM at calibrated mid on `initServoPwmDriver` with no NVS restore, releases PWM after idle `none`, and has no pre-upload hook. Control plane is `api/commands` + HTTP handlers + COBS serial `op` JSON (not the old path-style `api/dispatch`). Settings use the layered checklist in `docs/settings.md`. PlatformIO already needs Node for UI embed; CLI already speaks serial COBS via `serialRequest`.

## Goals / Non-Goals

**Goals:**

- Fit park into `api/commands` and both transports without reviving path-RPC dispatch.
- Soft OE-enable via saved pose; flash loop safety via park + hold.
- Pre-upload reuse CLI serial stack (no second COBS implementation).

**Non-Goals:**

- Park before web-flash or OTA (no reliable USB session).
- Persisting pose as a user-editable settings field.
- Changing limp delay when neither hold is active.
- Cherry-picking the old usb-serial-com commits.

## Decisions

### 1. Shared command: `apiParkServos()`

Add a transport-agnostic `ApiResult apiParkServos()` in `src/api/commands.*`: stop anim immediately, `ensureAllServoOutputs()`, `centerAllServos()`, flush pose (bool success), arm until-reboot hold. HTTP and serial both call it.

**Alternatives:** Duplicate logic in handler vs serial link — rejected. Revive `ops_servos` path dispatch — rejected (obsolete).

### 2. HTTP access: `Access::Token`

Route `POST /servos/park` with `Access::Token` so it works on setup AP and after provision (does not require `settingsProvisioned()`). Bearer still enforced when a token is set.

**Alternatives:** `Configured` — blocks unprovisioned flash-during-wizard. `Setup` — blocks after provision. New enum — unnecessary if Token fits.

### 3. Serial provisioning whitelist

In `handleReq`, allow `servos_park` alongside `ping`/`sync` while provisioning. Spec delta updates "Ping-only during provisioning".

### 4. Pose NVS: key `spose` in namespace `te`

Five `uint8_t` degrees, clamped to joint limits at save. Debounce ~500 ms via `pollServoPoseSave()` from `tickRobot`. Flush also on park and before limp release (when release is allowed). Factory reset clears namespace then rewrites kept calibration keys — do not rewrite `spose` (mid fallback next boot).

**Alternatives:** Separate Preferences namespace — more churn. Save only on park — weaker restore after crash mid-motion.

### 5. Hold stack

`g_parkHoldUntilReboot` OR `settingsServoHold()` → skip `releaseAllServoOutputs`. Park always sets the until-reboot flag. `servo_hold` is the persistent "stay stiff between flashes" preference (NVS `srv_hold`).

### 6. Boot vs `centerAllServos`

Restore pose (or mid) before OE LOW. Keep existing `main` path that may call `centerAllServos()` / sleep-inertia afterward — smooth move from restored pose. After a successful pre-upload park, restored pose is already mid so centering is a no-op.

### 7. CLI `park` + PIO Python wrapper

- CLI: `tiny-engineer park` → transport helper posts HTTP `/servos/park` or serial `servos_park`.
- PIO: `pre:scripts/park_servos_before_upload.py` registers `AddPreAction("upload", ...)`, resolves `node` + package CLI entry (repo-local `packages/tiny-engineer-cli/bin/tiny-engineer.js`), runs `park --transport serial` with short timeout, maps `$UPLOAD_PORT` to `--port` when set, prints warning and returns success on any failure so upload continues.

**Alternatives:** Pure Python COBS — duplicates protocol. HTTP-only park — misses USB-only / offline STA.

### 8. Settings / docs / UI sync

Follow `docs/settings.md` checklist for `servo_hold`. Sync new HTTP route per sync-api-endpoints rule. Document flash-safe park in `docs/flash.md` and `docs/hardware/servos.md`.

## Risks / Trade-offs

- [Pre-upload races with `pio device monitor` / exclusive CDC] → Fail-open; docs already say close monitor before upload.
- [Limp droop vs saved pose without park] → Restore can still snap; mitigate with park + `servo_hold` docs, not by disabling limp globally.
- [NVS wear from pose debounce] → Skip write when blob unchanged; 500 ms debounce.
- [Park available without connection mode] → Intentional for flash-during-wizard; still Token-gated on HTTP.
- [Repo-local CLI path in PIO script] → Document; if global `tiny-engineer` on PATH, prefer that then fall back to `node …/bin/tiny-engineer.js`.

## Migration Plan

- Additive firmware/CLI/docs. Existing devices: first boot without `spose` uses mid (same as today).
- No NVS migration for `servo_hold` (default false on missing key).
- Rollback: remove pre-script from `platformio.ini` and flash prior firmware; leftover `spose`/`srv_hold` keys are ignored by old builds.

## Open Questions

None material — deferred only: exact CLI help wording and whether Config UI places `servo_hold` next to `welcome` or under an advanced group (implementer choice, same setting).
