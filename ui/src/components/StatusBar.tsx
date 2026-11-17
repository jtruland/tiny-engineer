import { useStatus, useStatusMessage } from "../status/status";

export function StatusBar() {
  const status = useStatusMessage();
  const { clearStatus } = useStatus();

  if (!status) {
    return null;
  }

  return (
    <div id="status" className={status.type}>
      <span role="status">{status.message}</span>
      <button type="button" aria-label="Dismiss" onClick={clearStatus}>
        ×
      </button>
    </div>
  );
}
