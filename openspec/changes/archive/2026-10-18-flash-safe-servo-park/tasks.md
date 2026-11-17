# Tasks

## 1. Servo pose persistence

- [x] 1.1 Add NVS pose blob (`spose` in `te`): load/save helpers, debounce dirty flag, `flushServoPoseToNvs()` returning bool, `pollServoPoseSave()`; verify compile symbols link from a call site stub or `pio run` after 1.2
- [x] 1.2 Restore pose (else mid) in `initServoPwmDriver` before OE LOW; mark angles dirty on commanded moves; call `pollServoPoseSave()` from `tickRobot`; verify serial log path prints restore vs mid-fallback on boot (`pio run`)
- [x] 1.3 On factory reset, leave `spose` cleared (do not rewrite after `prefs.clear`); verify `docs/settings.md` factory-reset bullet mentions pose clear and `pio test -e native` still passes for existing settings tests

## 2. Hold + park command (firmware)

- [x] 2.1 Add until-reboot park hold + `settingsServoHold()` gate in `releaseAllServoOutputs`; verify limp skipped when either hold is active (code review / unit-free assert via serial log absence of `pwm off` when hold set)
- [x] 2.2 Implement `apiParkServos()` in `api/commands` (stop anim, ensure PWM, center, flush pose, arm hold; error if pose flush fails); verify `pio run`
- [x] 2.3 Wire `POST /servos/park` (`Access::Token`) in `kRoutes` + handler; wire serial op `servos_park` and whitelist it during provisioning; verify route table includes path and serial rejects other ops but accepts park while provisioning (logic review against `link.cpp` guard)

## 3. Setting `servo_hold`

- [x] 3.1 Add `servo_hold` through settings layers (`SETTINGS_DEFAULT_SERVO_HOLD`, NVS `srv_hold`, patch/validate/getters/save/reset/load) per `docs/settings.md` checklist; verify `pio test -e native` for validation/load behavior
- [x] 3.2 Expose `servo_hold` on HTTP settings GET/POST and serial `settings.get` / `settings.set`; verify GET JSON includes the field and POST toggles it
- [x] 3.3 Add Config UI control + `Settings` type + ApiReference `/settings` param row; verify `npm run lint --prefix ui` and `npm test --prefix ui`

## 4. CLI park + PlatformIO pre-upload

- [x] 4.1 Add transport helper + `tiny-engineer park` command (HTTP `POST /servos/park`, serial `servos_park`); verify `npm test --prefix packages/tiny-engineer-cli` covers success/error paths with mocks
- [x] 4.2 Add `scripts/park_servos_before_upload.py` (`AddPreAction("upload")`) that shells CLI with `--transport serial` (prefer PATH `tiny-engineer`, else repo `node packages/tiny-engineer-cli/bin/tiny-engineer.js`), maps `$UPLOAD_PORT`, fail-open; register in `platformio.ini`; verify script prints skip warning when no board and exits 0

## 5. Docs sync

- [x] 5.1 Document `POST /servos/park`, `servo_hold`, and boot pose restore in `docs/api.md`, `docs/serial.md`, `docs/flash.md`, `docs/hardware/servos.md`, `docs/settings.md`, `docs/hardware/testing.md`, and README settings blurb if listed; verify param tables match UI ApiReference exactly

## 6. Integration verification

- [x] 6.1 Run `pio run`, `pio test -e native`, CLI tests, and UI lint/test; verify all pass
- [x] 6.2 Manual bench (when hardware available): park via CLI serial, confirm hold, `pio run -t upload` shows park line then flash, boot restores mid without violent snap; if no hardware, note skip in apply summary

## Workflow follow-up

- Archive the change after review and apply are complete (`/opsx-archive`).
- Confirm archived specs land under `openspec/specs/` for `servo-pose` and `servos-park`, and deltas merge into `serial-protocol` / `cli-transport`.
