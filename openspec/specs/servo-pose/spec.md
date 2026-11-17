# servo-pose Specification

## Purpose

Persists the robot's last commanded servo angles across reboot so PWM can be re-enabled near the physical pose instead of snapping from unknown pulses.

## Requirements

### Requirement: Last commanded pose is stored in NVS
The device MUST store the last commanded angle for each servo joint in non-volatile storage as an internal pose record. The pose record MUST NOT appear as a settings API field that clients patch for calibration.

#### Scenario: Pose saved after motion settles
- **WHEN** commanded joint angles change and then remain still long enough for the debounce window
- **THEN** the device writes the five joint angles to the pose record

#### Scenario: Unchanged pose is not rewritten
- **WHEN** the debounced angles match the last successfully saved pose
- **THEN** the device does not perform a redundant NVS write

### Requirement: Boot restores saved pose before enabling outputs
On PWM driver initialization, while servo outputs are still disabled, the device MUST load the saved pose when present and valid, otherwise use each joint's calibrated mid. The device MUST write those pulses before enabling outputs.

#### Scenario: Saved pose present
- **WHEN** a valid pose record exists at boot
- **THEN** the device applies those angles before enabling PCA9685 outputs

#### Scenario: Missing or invalid pose
- **WHEN** the pose record is missing or invalid at boot
- **THEN** the device applies calibrated mid for every joint before enabling outputs

### Requirement: Factory reset clears saved pose
A factory reset MUST clear the saved pose record so the next boot uses mid fallback, without changing the existing keep-ranges / keep-RGB / keep-OLED-rotation factory-reset behavior for calibration fields.

#### Scenario: Reset then boot
- **WHEN** the user performs a factory reset and the device boots again
- **THEN** PWM init uses mid fallback because no valid pose remains
