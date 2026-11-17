import type { ComponentChildren } from "preact";

// The servo angle bar: `background` draws inside it, `children` sit on top (such as a slider).
export function ServoTrack({
  background,
  scale,
  children,
}: {
  background: ComponentChildren;
  scale: readonly number[];
  children?: ComponentChildren;
}) {
  return (
    <div className="servo-slider-wrap">
      <div className="servo-slider-track">
        <div className="servo-slider-bg">{background}</div>
        {children}
      </div>
      <div className="servo-scale">
        {scale.map((degrees) => (
          <span key={degrees}>{`${degrees}°`}</span>
        ))}
      </div>
    </div>
  );
}
