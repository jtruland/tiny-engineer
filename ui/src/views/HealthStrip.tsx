import { useEffect, useState } from "preact/hooks";
import { api } from "../api/client";
import type { Health } from "../api/types";
import { formatBytes, formatDuration } from "../lib/format";

function describeHealth(health: Health) {
  const heapUsed = health.heap_size
    ? Math.round((1 - health.free_heap / health.heap_size) * 100)
    : 0;

  return [
    `IP: ${health.wifi.connected ? health.wifi.ip : "offline"}`,
    `Uptime: ${formatDuration(health.uptime_ms)}`,
    `Heap: ${heapUsed}% used (${formatBytes(health.free_heap)} free)`,
    typeof health.cpu_temp_c === "number" && `Temp: ${health.cpu_temp_c.toFixed(1)} °C`,
    health.version,
  ]
    .filter(Boolean)
    .join(" · ");
}

export function HealthStrip() {
  const [text, setText] = useState("Loading status…");

  useEffect(() => {
    const controller = new AbortController();
    let timer: ReturnType<typeof setTimeout> | undefined;

    // The next poll starts once this one has answered, so a slow robot never gets overlapping requests.
    async function poll() {
      try {
        const health = await api<Health>("/health", { signal: controller.signal });

        if (!health.ok) {
          throw new Error(health.data.error);
        }

        setText(describeHealth(health.data));
      } catch {
        if (controller.signal.aborted) {
          return;
        }

        setText("Could not load status.");
      }

      timer = setTimeout(() => void poll(), 1000);
    }

    void poll();

    return () => {
      controller.abort();
      clearTimeout(timer);
    };
  }, []);

  return (
    <div id="health-info" className="info-strip">
      {text}
    </div>
  );
}
