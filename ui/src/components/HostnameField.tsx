export function HostnameField({
  id,
  value,
  onInput,
}: {
  id: string;
  value: string;
  onInput: (value: string) => void;
}) {
  return (
    <div className="form-group">
      <label htmlFor={id}>Hostname</label>
      <input
        type="text"
        id={id}
        maxLength={31}
        pattern="[A-Za-z0-9]([A-Za-z0-9-]{0,29}[A-Za-z0-9])?"
        required
        value={value}
        // Hostnames hold no spaces, so stray ones at either end are dropped while typing. The
        // field is written back too: when trimming leaves the state unchanged, nothing re-renders.
        onInput={(event) => {
          const trimmed = event.currentTarget.value.trim();

          event.currentTarget.value = trimmed;
          onInput(trimmed);
        }}
      />
      <p className="hint">Letters, digits, hyphen — no .local</p>
    </div>
  );
}
