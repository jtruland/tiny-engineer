import type { TargetedSubmitEvent } from "preact";
import { useState } from "preact/hooks";
import { Button } from "../components/Button";
import { type SettingsLoad, useRobot } from "../robot";

const ERRORS: Partial<Record<SettingsLoad, string>> = {
  unauthorized: "Invalid access token.",
  offline: "Could not reach the robot. Try again.",
};

export function AuthGate() {
  const { unlock } = useRobot();
  const [candidate, setCandidate] = useState("");
  const [error, setError] = useState<string | undefined>();

  async function submit(event: TargetedSubmitEvent<HTMLFormElement>) {
    event.preventDefault();
    setError(undefined);
    setError(ERRORS[await unlock(candidate)]);
  }

  return (
    <div className="gate">
      <div className="gate-box">
        <h1>Access token required</h1>
        <p>Enter the device access token to use the control panel.</p>
        <form onSubmit={submit}>
          <div className="form-group">
            <label htmlFor="auth-token">Access token</label>
            <input
              type="password"
              id="auth-token"
              autoComplete="current-password"
              required
              autoFocus
              value={candidate}
              onInput={(event) => setCandidate(event.currentTarget.value)}
            />
          </div>
          {error && (
            <div id="auth-error" role="alert">
              {error}
            </div>
          )}
          <Button type="submit" variant="primary">
            Unlock
          </Button>
        </form>
      </div>
    </div>
  );
}
