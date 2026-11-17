import { Button } from "../components/Button";
import { useStatus } from "../status/status";

const TESTS = [
  ["/test/audio", "Play ascending tones through the speaker.", "Audio tones"],
  ["/test/audio/bell", "Play the bell WAV file from storage.", "Bell sound"],
  ["/test/screen", "Draw a demo pattern on the OLED screen.", "OLED screen"],
  ["/test/movement", "Exercise all five servos through a sweep.", "All servos"],
  ["/test/led", "Cycle the onboard RGB LED through colors.", "RGB LED"],
] as const;

export function Tests() {
  const { perform } = useStatus();

  async function run(path: string) {
    await perform({ path, pending: "Running…", failure: "Request failed", done: "Done." });
  }

  return (
    <section>
      <h2 className="page-title">Hardware tests</h2>
      <p className="page-desc">Run built-in diagnostics. Each test blocks until complete.</p>
      {TESTS.map(([path, text, label]) => (
        <div key={path} className="test-card">
          <p>{text}</p>
          <Button onClick={() => run(path)}>{label}</Button>
        </div>
      ))}
    </section>
  );
}
