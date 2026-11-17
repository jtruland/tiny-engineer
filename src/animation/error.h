#pragma once

#include <cstdint>

// Stock error.wav: "Uh-oh. Human, we have a problem." (~2.23 s).
// /error.cue on LittleFS replaces these after the filesystem mounts.
extern uint32_t ERROR_AUDIO_UHOH_END_MS;
extern uint32_t ERROR_AUDIO_HUMAN_END_MS;
extern uint32_t ERROR_AUDIO_PROBLEM_END_MS;
extern uint32_t ERROR_AUDIO_END_MS;

void startError();
void startDeadWarning();
void updateError(uint32_t now);
// Obstacle pose + warning clip + glances. True when audio ends or fails to start.
bool updateErrorWarning(uint32_t now);
bool errorAudioStarted();
uint32_t errorAudioElapsed(uint32_t now);
