#pragma once

#include <cstdint>

// Stock attention.wav: "pst... human.... you might want to take a look" (~2.96 s).
// /attention.cue on LittleFS replaces these after the filesystem mounts.
extern uint32_t ATTENTION_AUDIO_PST_END_MS;
extern uint32_t ATTENTION_AUDIO_HUMAN_END_MS;
extern uint32_t ATTENTION_AUDIO_BLINK_START_MS;
extern uint32_t ATTENTION_AUDIO_BLINK_END_MS;
extern uint32_t ATTENTION_AUDIO_END_MS;

void startAttention();
void updateAttention(uint32_t now);
bool attentionAudioStarted();
uint32_t attentionAudioElapsed(uint32_t now);
