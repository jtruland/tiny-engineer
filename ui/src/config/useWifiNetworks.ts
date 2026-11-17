import { api } from "../api/client";
import type { Settings, WifiNetwork } from "../api/types";
import { useResettableState } from "../useResettableState";

export const MAX_NETWORKS = 5;

// Adds and removals wait here until the config form is saved.
export function useWifiNetworks(saved: string[], onSaved: (settings: Settings) => void) {
  const savedKey = saved.join("\n");
  const [removed, setRemoved] = useResettableState(savedKey, () => new Set<string>());
  const [added, setAdded] = useResettableState(savedKey, () => new Map<string, string>());

  // Boot tries the networks a scan misses in this order: new ones first, then the kept saved ones.
  const newNetworks = [...added].map(([ssid, password]) => ({ ssid, password }));
  const keptNetworks = saved
    .filter((ssid) => !removed.has(ssid) && !added.has(ssid))
    .map((ssid) => ({ ssid }));
  const desired: WifiNetwork[] = [...newNetworks, ...keptNetworks];
  const pending = removed.size > 0 || added.size > 0;

  function undoRemove(ssid: string) {
    setRemoved((names) => {
      const next = new Set(names);

      next.delete(ssid);

      return next;
    });
  }

  return {
    saved,
    added,
    removed,
    desired,
    pending,
    undoRemove,
    stageAdd: (ssid: string, password: string) => {
      undoRemove(ssid);
      setAdded((networks) => new Map(networks).set(ssid, password));
    },
    undoAdd: (ssid: string) => {
      setAdded((networks) => {
        const next = new Map(networks);

        next.delete(ssid);

        return next;
      });
    },
    remove: (ssid: string) => {
      setRemoved((names) => new Set(names).add(ssid));
    },
    // Resolves to the robot's error, or null when the list saved or nothing changed.
    save: async (): Promise<string | null> => {
      if (!pending) {
        return null;
      }

      let result;

      try {
        result = await api<Settings>("/settings/wifi", { method: "PUT", json: desired });
      } catch {
        return "network error";
      }

      if (!result.ok) {
        return result.data.error;
      }

      setRemoved(new Set());
      setAdded(new Map());
      onSaved(result.data);

      return null;
    },
  };
}
