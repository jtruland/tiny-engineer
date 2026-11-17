import { Button } from "../components/Button";
import type { useAccessToken } from "./useAccessToken";

type FieldProps = ReturnType<typeof useAccessToken>["field"];

export function AccessTokenField({ configured, value, clearPending, ...on }: FieldProps) {
  let placeholder = "Enter access token";
  let status = "Auth disabled";

  if (clearPending) {
    placeholder = "Token will be removed on Save";
    status = "Will remove on save";
  } else if (configured) {
    placeholder = "Enter a new token to replace";
    status = "Auth enabled — click field to replace";
  }

  return (
    <div className="form-group">
      <label htmlFor="config-access-token">Access token</label>
      <div className="token-row">
        <input
          type="password"
          id="config-access-token"
          maxLength={64}
          autoComplete="new-password"
          placeholder={placeholder}
          disabled={clearPending}
          value={value}
          onInput={(event) => on.onInput(event.currentTarget.value)}
          onFocus={on.onFocus}
          onBlur={on.onBlur}
        />
        <Button
          className="btn-token-toggle"
          hidden={!configured && !clearPending}
          onClick={on.onToggle}
        >
          {clearPending ? "Undo" : "Remove token"}
        </Button>
      </div>
      <p className="hint">{status}</p>
    </div>
  );
}
