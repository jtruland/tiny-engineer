import clsx from "clsx";
import type { ComponentChildren } from "preact";
import { ApplyBadge, type ApplyWhen } from "./ApplyBadge";

// `apply` puts a badge beside the title; `head` keeps the spaced title row without one.
export function ConfigSection({
  title,
  apply,
  head = apply !== undefined,
  className,
  children,
}: {
  title: string;
  apply?: ApplyWhen;
  head?: boolean;
  className?: string;
  children: ComponentChildren;
}) {
  return (
    <div className={clsx("config-section", className)}>
      {head ? (
        <div className="config-section-head">
          <h3>{title}</h3>
          {apply && <ApplyBadge when={apply} />}
        </div>
      ) : (
        <h3>{title}</h3>
      )}
      {children}
    </div>
  );
}
