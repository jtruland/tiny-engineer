# Spec Delta

## Purpose

Provides a flash-safe park command and PWM hold so joints sit at mid with torque before upload reset, and optionally stay held between flashes when limp droop would make restore unsafe.

## ADDED Requirements

### Requirement: Park centers joints and holds PWM until reboot
The device MUST expose a park operation that stops the current animation, moves every joint to its calibrated mid, persists the resulting pose, and keeps servo PWM enabled until the next reboot even if idle/sleep would otherwise release PWM.

#### Scenario: Successful park
- **WHEN** a client invokes park successfully
- **THEN** all joints end at calibrated mid, the pose record matches that mid, and PWM remains held until reboot

#### Scenario: Pose persist failure
- **WHEN** park completes the mid move but fails to persist the pose record
- **THEN** the device returns an error and still leaves PWM held until reboot

### Requirement: HTTP park endpoint
The device MUST accept `POST /servos/park` with no required body. When an access token is configured, the request MUST present a valid Bearer token. Park MUST be available in setup AP mode and after the device is provisioned (it MUST NOT require a chosen connection mode).

#### Scenario: Park over HTTP when provisioned
- **WHEN** a provisioned client sends `POST /servos/park` with valid auth
- **THEN** the device parks and returns success JSON including hold indication

#### Scenario: Park over HTTP on setup AP
- **WHEN** setup AP is active and a client sends `POST /servos/park` with valid auth
- **THEN** the device parks without requiring a connection mode

### Requirement: Serial park operation
The serial API MUST accept op `servos_park` with the same park semantics as `POST /servos/park`. Serial park MUST NOT require Bearer authentication.

#### Scenario: Park over serial when provisioned
- **WHEN** a provisioned host sends serial op `servos_park`
- **THEN** the device parks and returns a success response for that request id

### Requirement: Setting servo_hold skips idle limp
The settings surface MUST include boolean `servo_hold` (default false). When true, the device MUST NOT release PWM after idle/sleep park. The setting MUST apply immediately and MUST reset to default on factory reset.

#### Scenario: Enable hold via settings
- **WHEN** a client sets `servo_hold` to true
- **THEN** subsequent idle/sleep park leaves PWM enabled instead of full-off limp

#### Scenario: Default after factory reset
- **WHEN** factory reset completes
- **THEN** `servo_hold` is false

### Requirement: CLI park command
The CLI MUST provide a `park` command that invokes park over the selected transport (HTTP or serial) using the same transport resolution as other robot commands.

#### Scenario: Park over serial via CLI
- **WHEN** the user runs `tiny-engineer park --transport serial` against a reachable device
- **THEN** the CLI sends serial `servos_park` and reports success or the device error

#### Scenario: Park over HTTP via CLI
- **WHEN** the user runs `tiny-engineer park --transport http` against a reachable device
- **THEN** the CLI sends `POST /servos/park` and reports success or the device error

### Requirement: PlatformIO pre-upload parks over serial
Firmware upload via PlatformIO MUST attempt a pre-upload park by invoking the CLI park command with serial transport. If park fails (no port, timeout, busy CDC, cold board, missing Node/CLI), upload MUST continue after a clear warning.

#### Scenario: Board online before upload
- **WHEN** `pio run -t upload` runs and the robot answers serial park
- **THEN** joints are parked before the flash reset proceeds

#### Scenario: Board offline before upload
- **WHEN** `pio run -t upload` runs and park cannot complete
- **THEN** the pre-upload step warns and the upload continues
