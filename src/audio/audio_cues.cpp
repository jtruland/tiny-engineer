#include "audio/audio_cues.h"

#include <LittleFS.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "animation/abort.h"
#include "animation/attention.h"
#include "animation/dead.h"
#include "animation/error.h"
#include "animation/welcome.h"
#include "serial_log.h"

uint32_t WELCOME_AUDIO_GREETING_END_MS = 800;
uint32_t WELCOME_AUDIO_PAUSE_END_MS = 1340;
uint32_t WELCOME_AUDIO_BLINK_START_MS = 1020;
uint32_t WELCOME_AUDIO_BLINK_END_MS = 1120;
uint32_t WELCOME_AUDIO_QUESTION_END_MS = 2400;
uint32_t WELCOME_AUDIO_END_MS = 2680;

uint32_t ATTENTION_AUDIO_PST_END_MS = 640;
uint32_t ATTENTION_AUDIO_HUMAN_END_MS = 1540;
uint32_t ATTENTION_AUDIO_BLINK_START_MS = 720;
uint32_t ATTENTION_AUDIO_BLINK_END_MS = 780;
uint32_t ATTENTION_AUDIO_END_MS = 2960;

uint32_t ERROR_AUDIO_UHOH_END_MS = 460;
uint32_t ERROR_AUDIO_HUMAN_END_MS = 1060;
uint32_t ERROR_AUDIO_PROBLEM_END_MS = 2140;
uint32_t ERROR_AUDIO_END_MS = 2220;

uint32_t ABORT_AUDIO_FINE_END_MS = 520;
uint32_t ABORT_AUDIO_DIDNT_WANT_END_MS = 1350;
uint32_t ABORT_AUDIO_FINISH_END_MS = 2050;
uint32_t ABORT_AUDIO_END_MS = 2500;

uint32_t DEAD_AUDIO_SHUTDOWN_MS = 1700;
uint32_t DEAD_AUDIO_DENSE_MS = 1400;
uint32_t DEAD_AUDIO_END_MS = 3410;

namespace {

constexpr size_t kCueBytes = 512;
constexpr uint32_t kCueMaxMs = 600000;

struct CueField {
  const char* key;
  uint32_t* value;
};

void logCueRejected(const char* path) {
  serialLogPrint(path);
  serialLogPrintln(" invalid; stock marks kept");
}

bool parseUnsigned(const char* token, uint32_t* out) {
  if (token == nullptr || token[0] == '\0') {
    return false;
  }

  for (const char* cursor = token; *cursor != '\0'; cursor++) {
    if (*cursor < '0' || *cursor > '9') {
      return false;
    }
  }

  char* end = nullptr;
  const unsigned long parsed = strtoul(token, &end, 10);

  if (end == token || *end != '\0' || parsed == 0 || parsed > kCueMaxMs) {
    return false;
  }

  *out = static_cast<uint32_t>(parsed);
  return true;
}

bool readCueFile(
  const char* path,
  CueField* fields,
  size_t count
) {
  File file = LittleFS.open(path, "r");

  if (!file) {
    logCueRejected(path);
    return false;
  }

  char buffer[kCueBytes];
  const size_t bytes = file.readBytes(buffer, sizeof(buffer) - 1);
  const bool tooLong = file.available();
  file.close();

  if (tooLong || bytes == 0) {
    logCueRejected(path);
    return false;
  }

  buffer[bytes] = '\0';

  bool seen[8] = {};

  if (count > 8) {
    logCueRejected(path);
    return false;
  }

  char* cursor = buffer;

  while (*cursor != '\0') {
    char* line = cursor;

    while (*cursor != '\0' && *cursor != '\n') {
      cursor++;
    }

    if (*cursor == '\n') {
      *cursor = '\0';
      cursor++;
    }

    size_t length = strlen(line);

    if (length > 0 && line[length - 1] == '\r') {
      line[length - 1] = '\0';
    }

    char* comment = strchr(line, '#');

    if (comment != nullptr) {
      *comment = '\0';
    }

    while (*line == ' ' || *line == '\t') {
      line++;
    }

    if (*line == '\0') {
      continue;
    }

    char* equals = strchr(line, '=');

    if (equals == nullptr || equals == line) {
      logCueRejected(path);
      return false;
    }

    *equals = '\0';
    char* key = line;
    char* value = equals + 1;

    while (*value == ' ' || *value == '\t') {
      value++;
    }

    size_t valueLength = strlen(value);

    while (valueLength > 0 &&
           (value[valueLength - 1] == ' ' || value[valueLength - 1] == '\t')) {
      value[--valueLength] = '\0';
    }

    size_t keyLength = strlen(key);

    while (keyLength > 0 &&
           (key[keyLength - 1] == ' ' || key[keyLength - 1] == '\t')) {
      key[--keyLength] = '\0';
    }

    size_t match = count;

    for (size_t i = 0; i < count; i++) {
      if (strcmp(fields[i].key, key) == 0) {
        match = i;
        break;
      }
    }

    if (match == count || seen[match]) {
      logCueRejected(path);
      return false;
    }

    uint32_t parsed = 0;

    if (!parseUnsigned(value, &parsed)) {
      logCueRejected(path);
      return false;
    }

    *fields[match].value = parsed;
    seen[match] = true;
  }

  for (size_t i = 0; i < count; i++) {
    if (!seen[i]) {
      logCueRejected(path);
      return false;
    }
  }

  return true;
}

bool increasing(const uint32_t* values, size_t count) {
  uint32_t previous = 0;

  for (size_t i = 0; i < count; i++) {
    if (values[i] <= previous) {
      return false;
    }

    previous = values[i];
  }

  return true;
}

bool blinkInside(uint32_t start, uint32_t end, uint32_t lo, uint32_t hi) {
  return lo <= start && start < end && end <= hi;
}

void loadWelcomeCue() {
  uint32_t greeting = WELCOME_AUDIO_GREETING_END_MS;
  uint32_t pause = WELCOME_AUDIO_PAUSE_END_MS;
  uint32_t blinkStart = WELCOME_AUDIO_BLINK_START_MS;
  uint32_t blinkEnd = WELCOME_AUDIO_BLINK_END_MS;
  uint32_t question = WELCOME_AUDIO_QUESTION_END_MS;
  uint32_t end = WELCOME_AUDIO_END_MS;
  const char* path = "/welcome.cue";

  if (!LittleFS.exists(path)) {
    return;
  }

  CueField fields[] = {
    {"greeting_end_ms", &greeting},
    {"pause_end_ms", &pause},
    {"blink_start_ms", &blinkStart},
    {"blink_end_ms", &blinkEnd},
    {"question_end_ms", &question},
    {"end_ms", &end},
  };

  if (!readCueFile(path, fields, 6)) {
    return;
  }

  const uint32_t order[] = {greeting, pause, question, end};

  if (!increasing(order, 4) || !blinkInside(blinkStart, blinkEnd, greeting, pause)) {
    logCueRejected(path);
    return;
  }

  WELCOME_AUDIO_GREETING_END_MS = greeting;
  WELCOME_AUDIO_PAUSE_END_MS = pause;
  WELCOME_AUDIO_BLINK_START_MS = blinkStart;
  WELCOME_AUDIO_BLINK_END_MS = blinkEnd;
  WELCOME_AUDIO_QUESTION_END_MS = question;
  WELCOME_AUDIO_END_MS = end;
  serialLogPrintln("welcome.cue loaded");
}

void loadAttentionCue() {
  uint32_t pst = ATTENTION_AUDIO_PST_END_MS;
  uint32_t human = ATTENTION_AUDIO_HUMAN_END_MS;
  uint32_t blinkStart = ATTENTION_AUDIO_BLINK_START_MS;
  uint32_t blinkEnd = ATTENTION_AUDIO_BLINK_END_MS;
  uint32_t end = ATTENTION_AUDIO_END_MS;
  const char* path = "/attention.cue";

  if (!LittleFS.exists(path)) {
    return;
  }

  CueField fields[] = {
    {"pst_end_ms", &pst},
    {"human_end_ms", &human},
    {"blink_start_ms", &blinkStart},
    {"blink_end_ms", &blinkEnd},
    {"end_ms", &end},
  };

  if (!readCueFile(path, fields, 5)) {
    return;
  }

  const uint32_t order[] = {pst, human, end};

  if (!increasing(order, 3) || !blinkInside(blinkStart, blinkEnd, pst, human)) {
    logCueRejected(path);
    return;
  }

  ATTENTION_AUDIO_PST_END_MS = pst;
  ATTENTION_AUDIO_HUMAN_END_MS = human;
  ATTENTION_AUDIO_BLINK_START_MS = blinkStart;
  ATTENTION_AUDIO_BLINK_END_MS = blinkEnd;
  ATTENTION_AUDIO_END_MS = end;
  serialLogPrintln("attention.cue loaded");
}

void loadErrorCue() {
  uint32_t uhoh = ERROR_AUDIO_UHOH_END_MS;
  uint32_t human = ERROR_AUDIO_HUMAN_END_MS;
  uint32_t problem = ERROR_AUDIO_PROBLEM_END_MS;
  uint32_t end = ERROR_AUDIO_END_MS;
  const char* path = "/error.cue";

  if (!LittleFS.exists(path)) {
    return;
  }

  CueField fields[] = {
    {"uhoh_end_ms", &uhoh},
    {"human_end_ms", &human},
    {"problem_end_ms", &problem},
    {"end_ms", &end},
  };

  if (!readCueFile(path, fields, 4)) {
    return;
  }

  const uint32_t order[] = {uhoh, human, problem, end};

  if (!increasing(order, 4)) {
    logCueRejected(path);
    return;
  }

  ERROR_AUDIO_UHOH_END_MS = uhoh;
  ERROR_AUDIO_HUMAN_END_MS = human;
  ERROR_AUDIO_PROBLEM_END_MS = problem;
  ERROR_AUDIO_END_MS = end;
  serialLogPrintln("error.cue loaded");
}

void loadAbortCue() {
  uint32_t fine = ABORT_AUDIO_FINE_END_MS;
  uint32_t didntWant = ABORT_AUDIO_DIDNT_WANT_END_MS;
  uint32_t finish = ABORT_AUDIO_FINISH_END_MS;
  uint32_t end = ABORT_AUDIO_END_MS;
  const char* path = "/abort.cue";

  if (!LittleFS.exists(path)) {
    return;
  }

  CueField fields[] = {
    {"fine_end_ms", &fine},
    {"didnt_want_end_ms", &didntWant},
    {"finish_end_ms", &finish},
    {"end_ms", &end},
  };

  if (!readCueFile(path, fields, 4)) {
    return;
  }

  const uint32_t order[] = {fine, didntWant, finish, end};

  if (!increasing(order, 4)) {
    logCueRejected(path);
    return;
  }

  ABORT_AUDIO_FINE_END_MS = fine;
  ABORT_AUDIO_DIDNT_WANT_END_MS = didntWant;
  ABORT_AUDIO_FINISH_END_MS = finish;
  ABORT_AUDIO_END_MS = end;
  serialLogPrintln("abort.cue loaded");
}

void loadDeadCue() {
  uint32_t shutdown = DEAD_AUDIO_SHUTDOWN_MS;
  uint32_t dense = DEAD_AUDIO_DENSE_MS;
  uint32_t end = DEAD_AUDIO_END_MS;
  const char* path = "/dead.cue";

  if (!LittleFS.exists(path)) {
    return;
  }

  CueField fields[] = {
    {"shutdown_ms", &shutdown},
    {"dense_ms", &dense},
    {"end_ms", &end},
  };

  if (!readCueFile(path, fields, 3)) {
    return;
  }

  const uint32_t order[] = {dense, shutdown, end};

  if (!increasing(order, 3)) {
    logCueRejected(path);
    return;
  }

  DEAD_AUDIO_SHUTDOWN_MS = shutdown;
  DEAD_AUDIO_DENSE_MS = dense;
  DEAD_AUDIO_END_MS = end;
  serialLogPrintln("dead.cue loaded");
}

}  // namespace

void loadAudioCues() {
  loadWelcomeCue();
  loadAttentionCue();
  loadErrorCue();
  loadAbortCue();
  loadDeadCue();
}
