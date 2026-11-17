# Spec Delta

## MODIFIED Requirements

### Requirement: Explicit transport selection
The CLI MUST resolve transport in this order: command-line flag, then environment sources for `TINY_ENGINEER_TRANSPORT` (process env, then project `.env`, then user config `env`, then other documented dotenv sources), then the documented default `http`. The CLI MUST NOT choose between HTTP and serial by probing reachability or preferring a plugged USB device. A transport value written to the user config `env` by setup counts as an environment selection for later commands when not overridden by a higher-precedence source.

#### Scenario: Serial transport selected
- **WHEN** the user runs a robot command with transport `serial`
- **THEN** the CLI uses the serial protocol and does not fall back to HTTP on failure

#### Scenario: HTTP transport selected
- **WHEN** the user runs a robot command with transport `http`
- **THEN** the CLI uses HTTP and does not fall back to serial on failure

#### Scenario: Unset transport defaults to HTTP
- **WHEN** neither a transport flag nor a transport environment variable is set
- **THEN** the CLI uses HTTP (backward compatible) and does not probe serial

#### Scenario: Flag overrides env
- **WHEN** an env source sets `TINY_ENGINEER_TRANSPORT=serial` and the user passes `--transport http`
- **THEN** the CLI uses HTTP for that invocation

#### Scenario: Legacy project env still overrides user config
- **WHEN** project `.env` sets `TINY_ENGINEER_TRANSPORT=serial` and user `env` sets `http`
- **THEN** the CLI uses serial when no transport flag is passed

### Requirement: Project setup chooses host transport
The `tiny-engineer setup` wizard MUST let the user choose host transport `http` or `serial` via an interactive prompt, or via non-interactive flags/env with the same meaning.

#### Scenario: Interactive serial setup
- **WHEN** the user runs interactive setup and chooses serial transport
- **THEN** setup records serial as the chosen host transport in user config

### Requirement: Project setup merges transport into .env
Setup MUST merge `TINY_ENGINEER_TRANSPORT` into the user config `env` file (not the project `.env`) using create / confirm-on-diff / keep-on-`--yes` behavior defined by the cli-user-config capability.

#### Scenario: Serial transport written to env
- **WHEN** setup completes with serial transport selected and no conflicting stored value blocks the write
- **THEN** the user config `env` contains `TINY_ENGINEER_TRANSPORT=serial`

#### Scenario: Commands use setup-written transport
- **WHEN** setup has written `TINY_ENGINEER_TRANSPORT=serial` to user config and the user runs `anim` without a transport flag and without a higher-precedence override
- **THEN** the CLI uses the serial transport

### Requirement: Project setup HTTP still collects URL
When the chosen host transport is `http`, setup MUST collect a robot base URL and merge `TINY_ENGINEER_URL` into the user config `env`, and MUST also persist transport as `http` there (subject to confirm-on-diff / `--yes` keep rules).

#### Scenario: Interactive HTTP setup writes URL and transport
- **WHEN** the user runs interactive setup, chooses HTTP, and provides a base URL with no conflicting stored values (or confirms replace)
- **THEN** the user config `env` contains `TINY_ENGINEER_TRANSPORT=http` and `TINY_ENGINEER_URL` for that URL

### Requirement: Project setup serial does not require URL
When the chosen host transport is `serial`, setup MUST NOT require a robot URL. Setup MAY offer an optional serial port and MUST merge `TINY_ENGINEER_PORT` into the user config `env` when the user provides one (subject to confirm-on-diff / `--yes` keep rules).

#### Scenario: Serial setup without URL
- **WHEN** the user completes serial transport setup without entering a URL
- **THEN** setup succeeds and does not fail for a missing `TINY_ENGINEER_URL`

## REMOVED Requirements

### Requirement: Uninstall purges transport env keys
**Reason**: Setup no longer writes connection keys to project `.env`; connection prefs live in user config; uninstall should only remove IDE hooks. `--purge-env` is dropped.
**Migration**: Edit or delete the user config `env` file and any legacy project `.env` Tiny Engineer keys by hand. Uninstall continues to remove hook wiring.
