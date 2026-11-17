export function RebootGate() {
  return (
    <div className="gate">
      <div className="gate-box">
        <h1>Factory reset done</h1>
        <p>
          Settings and WiFi credentials are cleared. Servo ranges, RGB LED mapping, and screen
          rotation stay. Power-cycle or press the device reset button.
        </p>
        <p>
          Then join the robot WiFi shown on the OLED and open the setup page to configure home WiFi.
          You can retune servo ranges, screen rotation, and LED mapping there.
        </p>
      </div>
    </div>
  );
}
