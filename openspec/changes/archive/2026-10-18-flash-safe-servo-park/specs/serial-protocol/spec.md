# Spec Delta

## MODIFIED Requirements

### Requirement: Ping-only during provisioning
While the device is unprovisioned or in setup AP provisioning, the serial API MUST accept only sync/ping (identity) operations and `servos_park`, and MUST reject other control operations with a clear error.

#### Scenario: Ping during setup AP
- **WHEN** the setup AP is active and the host sends a sync/ping request
- **THEN** the device responds with identity information indicating provisioning state

#### Scenario: Anim rejected while provisioning
- **WHEN** the setup AP is active and the host sends a serial animation request
- **THEN** the device rejects the request without changing animation state

#### Scenario: Park allowed while provisioning
- **WHEN** the setup AP is active and the host sends serial op `servos_park`
- **THEN** the device performs park and does not reject solely for provisioning state

### Requirement: Feature parity with HTTP control surface
The serial API MUST expose the same day-to-day control operations as the HTTP JSON API, including animations, settings read/write, hardware tests, servo test, health, factory reset, streamed WAV play with optional animation name, and servo park.

#### Scenario: Settings over serial
- **WHEN** the host requests current settings over serial
- **THEN** the device returns the same settings fields exposed by `GET /settings` (excluding secrets)

#### Scenario: Play with pose over serial
- **WHEN** the host streams a WAV with an allowed play animation name over serial
- **THEN** the device plays the clip while running that animation, matching `POST /play` behavior

#### Scenario: Park over serial matches HTTP
- **WHEN** the host sends serial op `servos_park`
- **THEN** the device applies the same park semantics as `POST /servos/park`
