import { useState } from "preact/hooks";

// Local state that starts from `source` and starts over whenever `source` changes,
// such as a form draft that follows the settings the robot last returned.
export function useResettableState<S, T>(source: S, initial: (source: S) => T) {
  const [state, setState] = useState(() => initial(source));
  const [seen, setSeen] = useState(source);

  if (!Object.is(seen, source)) {
    setSeen(source);
    setState(initial(source));
  }

  return [state, setState] as const;
}
