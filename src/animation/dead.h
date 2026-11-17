#pragma once

#include <cstdint>

// Eyes: squeeze nearly shut after warning, then brief hold before X X.
constexpr uint32_t DEAD_SQUEEZE_MS = 300;
constexpr uint32_t DEAD_PAUSE_MS = 200;
constexpr uint32_t DEAD_COLLAPSE_MS = 500;

// Stock dead.wav (~3.41 s): "Insufficient resources," then "shutting down..." at 1700 ms.
// /dead.cue on LittleFS replaces these after the filesystem mounts.
extern uint32_t DEAD_AUDIO_SHUTDOWN_MS;
extern uint32_t DEAD_AUDIO_DENSE_MS;
extern uint32_t DEAD_AUDIO_END_MS;

void startDead();
void updateDead(uint32_t now);
bool deadShowingX();
bool deadIsSqueezing();
bool deadIsPausedShut();
uint32_t deadSqueezeStartedMs();
