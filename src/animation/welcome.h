#pragma once

#include <stdint.h>

// Stock welcome.wav: "Hello, human. … What are we building today?" (~2.68 s).
// /welcome.cue on LittleFS replaces these after the filesystem mounts.
extern uint32_t WELCOME_AUDIO_GREETING_END_MS;
extern uint32_t WELCOME_AUDIO_PAUSE_END_MS;
extern uint32_t WELCOME_AUDIO_BLINK_START_MS;
extern uint32_t WELCOME_AUDIO_BLINK_END_MS;
extern uint32_t WELCOME_AUDIO_QUESTION_END_MS;
extern uint32_t WELCOME_AUDIO_END_MS;

void startWelcome();
void updateWelcome(uint32_t now);
bool welcomeAudioStarted();
uint32_t welcomeAudioElapsed(uint32_t now);
