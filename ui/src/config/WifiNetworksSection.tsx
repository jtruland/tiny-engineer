import clsx from "clsx";
import type { TargetedKeyboardEvent } from "preact";
import { useState } from "preact/hooks";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { useStatus } from "../status/status";
import { MAX_NETWORKS, type useWifiNetworks } from "./useWifiNetworks";

type WifiNetworks = ReturnType<typeof useWifiNetworks>;

function NetworkRow({
  ssid,
  note = "",
  label,
  removed = false,
  enabled = true,
  onClick,
}: {
  ssid: string;
  note?: string;
  label: "Undo" | "Remove";
  removed?: boolean;
  enabled?: boolean;
  onClick: () => void;
}) {
  return (
    <li className={clsx("wifi-network", removed && "wifi-network-removed")} aria-label={ssid}>
      <span className="wifi-network-ssid">{ssid}</span>
      <span className="wifi-network-note">{note}</span>
      <Button
        variant={label === "Remove" ? "danger" : undefined}
        disabled={!enabled}
        onClick={onClick}
      >
        {label}
      </Button>
    </li>
  );
}

export function WifiNetworksSection({ networks }: { networks: WifiNetworks }) {
  const { setStatus } = useStatus();
  const [ssid, setSsid] = useState("");
  const [password, setPassword] = useState("");

  function stageAdd() {
    const name = ssid.trim();

    if (!name) {
      setStatus("Enter a WiFi network name.", "err");

      return;
    }

    const listed = networks.desired.some((network) => network.ssid === name);

    if (!listed && networks.desired.length >= MAX_NETWORKS) {
      setStatus(`The robot keeps up to ${MAX_NETWORKS} networks. Remove one first.`, "err");

      return;
    }

    networks.stageAdd(name, password);
    setSsid("");
    setPassword("");
  }

  const addOnEnter = (event: TargetedKeyboardEvent<HTMLInputElement>) => {
    if (event.key === "Enter") {
      event.preventDefault();
      stageAdd();
    }
  };

  return (
    <ConfigSection title="WiFi networks" apply="reboot">
      <p className="config-section-desc">
        At boot the robot joins the strongest saved network in range. Up to 5 networks. Changes are
        sent on Save. The robot cannot test a network from here, so check the password.
      </p>
      <ul className="wifi-network-list" aria-label="Saved WiFi networks">
        {[...networks.added.keys()].map((name) => (
          <NetworkRow
            key={name}
            ssid={name}
            note={networks.saved.includes(name) ? "New password on save" : "Added on save"}
            label="Undo"
            onClick={() => networks.undoAdd(name)}
          />
        ))}
        {networks.saved
          .filter((name) => !networks.added.has(name))
          .map((name) =>
            networks.removed.has(name) ? (
              <NetworkRow
                key={name}
                ssid={name}
                note="Removed on save"
                label="Undo"
                removed
                onClick={() => networks.undoRemove(name)}
              />
            ) : (
              // The firmware keeps its last network; factory reset is the way to clear WiFi.
              <NetworkRow
                key={name}
                ssid={name}
                label="Remove"
                enabled={networks.desired.length > 1}
                onClick={() => networks.remove(name)}
              />
            ),
          )}
      </ul>
      <div className="form-group">
        <label htmlFor="wifi-network-ssid">Network name (SSID)</label>
        <input
          type="text"
          id="wifi-network-ssid"
          maxLength={32}
          autoComplete="off"
          value={ssid}
          onInput={(event) => setSsid(event.currentTarget.value)}
          onKeyDown={addOnEnter}
        />
      </div>
      <div className="form-group">
        <label htmlFor="wifi-network-password">Password</label>
        <input
          type="password"
          id="wifi-network-password"
          maxLength={63}
          autoComplete="new-password"
          value={password}
          onInput={(event) => setPassword(event.currentTarget.value)}
          onKeyDown={addOnEnter}
        />
        <p className="hint">Leave empty only for open networks.</p>
      </div>
      <Button id="wifi-network-add" onClick={stageAdd}>
        Add network
      </Button>
    </ConfigSection>
  );
}
