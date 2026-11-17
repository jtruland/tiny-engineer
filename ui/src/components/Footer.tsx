import { useRobot } from "../robot";

export function Footer() {
  const { version } = useRobot();

  return (
    <footer>
      <a href="https://tinyengineer.dev" target="_blank" rel="noopener">
        tinyengineer.dev
      </a>
      <p id="fw-version">{version ? `Firmware ${version}` : "Firmware …"}</p>
    </footer>
  );
}
