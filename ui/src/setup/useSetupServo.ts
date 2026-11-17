import { useStatus } from "../status/status";

export function useSetupServo() {
  const { perform } = useStatus();

  return async (json: { all: number } | { index: number; angle: number }) => {
    const result = await perform({
      path: "/setup/servo",
      request: { method: "POST", json },
      pending: "Moving servos…",
      failure: "Move failed",
    });

    return result?.ok === true;
  };
}
