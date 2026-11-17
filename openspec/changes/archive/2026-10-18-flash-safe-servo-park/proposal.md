# Proposal

## Why

USB flash resets the ESP32 while servos may be limp or mid-pose. On reboot, enabling PCA9685 OE with mid/default pulses snaps joints against printed parts. A prior branch had park-before-upload plus last-pose restore, but it never landed on `main` and used an obsolete serial path API. Flash loops on the current COBS/`api/commands` stack still need that safety, reimplemented for today's architecture.

## What Changes

- Add `POST /servos/park`: stop animation, smooth-center all joints, flush last pose to NVS, hold PWM until reboot (NVS pose write failure returns an error).
- Add serial op `servos_park` with the same behavior; allow it during setup AP / unprovisioned (exception to ping-only), so flash during wizard still parks.
- Persist last commanded servo angles in NVS (`spose`); restore them in `initServoPwmDriver` before OE goes LOW (mid fallback if missing/invalid).
- Debounced pose save during runtime; skip idle/sleep PWM full-off when park hold is armed or setting `servo_hold` is true.
- Add settings field `servo_hold` (default false, immediate apply; factory reset restores default; `spose` cleared on factory reset, ranges kept as today).
- Add CLI `tiny-engineer park` (HTTP + serial transports).
- Add PlatformIO pre-upload hook that shells the CLI with `--transport serial` (fail-open: warn and continue if board cold/busy/missing).
- Sync docs and web UI API/settings surfaces (api, serial, flash, servos, settings checklist, ApiReference, Config).

No **BREAKING** API removals. Additive route, op, setting, and CLI command.

## Capabilities

### New Capabilities

- `servo-pose`: Persist last commanded joint angles, restore them at PWM init before OE enable, and debounce NVS writes.
- `servos-park`: Park command (HTTP + serial + CLI), until-reboot PWM hold, `servo_hold` setting, and PlatformIO pre-upload park via CLI.

### Modified Capabilities

- `serial-protocol`: Allow `servos_park` during provisioning; extend feature-parity list to include park.
- `cli-transport`: Day-to-day CLI includes `park`; pre-upload may invoke it over serial.

## Impact

- Firmware: `api/commands`, HTTP `kRoutes`/handlers, `serial/link.cpp`, `servo_wrapper` / PCA9685 init, `robot_tick`, settings layers (`srv_hold`, `spose`).
- Host: `packages/tiny-engineer-cli` (`park` command + transport helper), `platformio.ini` + `scripts/park_servos_before_upload.py`.
- Docs/UI: `docs/api.md`, `docs/serial.md`, `docs/flash.md`, `docs/hardware/servos.md`, `docs/settings.md`, `docs/hardware/testing.md`, README if settings summary lists fields, `ui` Config + ApiReference.
- Tests: CLI unit tests for park transport; native settings validation for `servo_hold` if applicable; host script smoke if practical.
- Out of scope: web-flash / OTA pre-park (no USB serial hook); changing default limp timing when neither hold is active.
