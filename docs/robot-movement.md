# Robot layout and movement

The robot sits in a chair beside a desk. A miniature laptop and a service bell sit on the desk in front of it. Five micro servos drive the pose; limits and channel IDs are in [`include/servos.h`](../include/servos.h).

| Index | PCA channel | Servo | Motion | At `min` | At `max` | Safe range (°) |
|------:|------------:|-------|--------|----------|----------|----------------|
| 0 | 4 | **Head** | Face up / down (pitch) | down | up | 50 – 130 |
| 1 | 0 | **Neck** | Head left / right (yaw) | left | right | 30 – 150 |
| 2 | 3 | **Left hand** | Hand up / down | down | up | 40 – 110 |
| 3 | 1 | **Right hand** | Hand up / down (inverted scale) | up | down | 60 – 130 |
| 4 | 2 | **Body** | Whole torso left / right | right | left | 36 – 89 |

⚠️ **Index is not the PCA9685 channel** on this build — the harnesses were attached without tracking which output went where, so `SERVO_SPECS[].channel` carries the real mapping. `POST /test/servo?index=N` takes the index.

⚠️ **The neck's direction is reversed from stock.** Its horn was remounted 180° to bring the head forward at 90°, which flips which way increasing angle turns it — hence `min` = left here where stock had `min` = right. Verify against an animation before trusting it.

Ranges are this robot's measured calibration, not the stock figures. Those values are **defaults**. After setup AP calibration they are stored in NVS. Factory reset keeps the saved ranges. Retune them in the setup wizard when AP mode is open.

Animations author poses in **−1..1** relative to each joint’s saved min/max (`−1` = min, `0` = mid, `+1` = max). Horn offset after assembly is absorbed by calibration; firmware maps those poses with `servoNormToDeg`. Right-hand rest is still electrical min (`−1`); left-hand rest is electrical max (`+1`).

**Head** tilts the face toward or away from the laptop. **Neck** pans the head side to side. **Hands** lift and lower over the keyboard; the forearms have no elbow servo — the arm linkage is fixed, so only the hand joint moves. **Left** and **right** hand servos use opposite scales: on the left, higher angle is up; on the right, higher angle is down (rest pose for typing is left at `max`, right at `min`). **Body** rotates the whole upper body in the chair while the base stays put.

Command a single joint for bench checks:

```bash
curl -X POST "http://tiny-engineer.local/test/servo?index=0&angle=90"
```

Use angles inside the saved safe range on the assembled robot. During setup AP, `POST /setup/servo` can command the full 0–180° electrical range so you can find those limits.

Related: [servos.md](hardware/servos.md) (PWM and electrical limits), [api.md](api.md) (`/test/servo`, `/anim`).
