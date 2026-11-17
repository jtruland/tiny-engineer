#pragma once

#include <cstdint>

// Stock abort.wav: "Fine. I didn't want to finish it anyway." (2.5 s).
// /abort.cue on LittleFS replaces these after the filesystem mounts.
extern uint32_t ABORT_AUDIO_FINE_END_MS;
extern uint32_t ABORT_AUDIO_DIDNT_WANT_END_MS;
extern uint32_t ABORT_AUDIO_FINISH_END_MS;
extern uint32_t ABORT_AUDIO_END_MS;

void startAbort();
void updateAbort(uint32_t now);
bool abortAudioStarted();
uint32_t abortAudioElapsed(uint32_t now);
