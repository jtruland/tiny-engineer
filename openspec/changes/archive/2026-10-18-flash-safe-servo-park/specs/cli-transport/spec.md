# Spec Delta

## MODIFIED Requirements

### Requirement: Day-to-day command parity with web UI
Over the selected transport, the CLI MUST provide commands equivalent to the day-to-day web UI: animations, WAV play, settings get/set, hardware tests, servo test positioning, health, factory reset (with confirmation), and servo park. Setup-only calibration that remains AP-only in firmware MAY stay out of the CLI.

#### Scenario: Settings via CLI
- **WHEN** the user runs a settings get command with a valid transport
- **THEN** the CLI prints settings returned by the robot

#### Scenario: Hardware test via CLI
- **WHEN** the user runs a supported hardware test command
- **THEN** the CLI invokes the corresponding robot test operation

#### Scenario: Park via CLI
- **WHEN** the user runs the park command with a valid transport
- **THEN** the CLI invokes the robot park operation over that transport

## ADDED Requirements

### Requirement: Pre-upload may invoke CLI park over serial
PlatformIO pre-upload automation MUST be able to invoke the CLI park command with serial transport without requiring project `.env` transport to be serial. Failure of that park attempt MUST NOT abort the upload.

#### Scenario: Explicit serial transport for pre-upload
- **WHEN** the PlatformIO pre-upload hook runs park for flash safety
- **THEN** it requests serial transport explicitly rather than relying on the project's default transport
