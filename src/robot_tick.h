#pragma once

// One pass of the robot's own work (Wi-Fi, animation, eyes, sleep, LED) without
// serving HTTP or OTA. loop() calls it, and so does POST /play while it holds
// the server streaming a clip.
void tickRobot();
