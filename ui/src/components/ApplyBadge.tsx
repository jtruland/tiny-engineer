export type ApplyWhen = "now" | "reboot";

export function ApplyBadge({ when }: { when: ApplyWhen }) {
  return (
    <span className={`apply-badge apply-${when}`}>
      {when === "now" ? "Immediate" : "After reboot"}
    </span>
  );
}
