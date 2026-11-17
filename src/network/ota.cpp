#include "network/ota.h"

#ifdef TE_OTA

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <esp_ota_ops.h>

#include <cstring>
#include <memory>

#include "audio/audio.h"
#include "display/oled.h"
#include "settings/settings.h"
#include "serial_log.h"

namespace {

// ArduinoOTAClass cannot drop a password once set, so a token change rebuilds the instance.
std::unique_ptr<ArduinoOTAClass> g_ota;
char g_token[SETTINGS_ACCESS_TOKEN_MAX_LEN + 1] = {};
int g_lastPercent = -1;

void showOtaProgress(unsigned int progress, unsigned int total) {
  const int percent = total > 0 ? (int)(progress * 100ULL / total) : 0;

  if (percent == g_lastPercent) {
    return;
  }

  g_lastPercent = percent;

  char line[8];
  snprintf(line, sizeof(line), "%d%%", percent);
  showOledText("OTA UPDATE", line);
}

void confirmRunningFirmware() {
  esp_ota_img_states_t state;

  if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) != ESP_OK ||
      state != ESP_OTA_IMG_PENDING_VERIFY) {
    return;
  }

  esp_ota_mark_app_valid_cancel_rollback();
  serialLogPrintln("OTA: new firmware confirmed");
}

}  // namespace

// Defers the Arduino core's boot-time confirmation of fresh OTA firmware until startOta() proves
// it can reach Wi-Fi and accept the next update. Unconfirmed firmware is rolled back on reset.
extern "C" bool verifyRollbackLater() {
  return true;
}

void startOta() {
  strncpy(g_token, settingsAccessToken(), SETTINGS_ACCESS_TOKEN_MAX_LEN);
  g_token[SETTINGS_ACCESS_TOKEN_MAX_LEN] = '\0';

  g_ota = std::make_unique<ArduinoOTAClass>();
  g_ota->setHostname(settingsHostname());
  // wifi_connect owns mDNS; ArduinoOTA::end() would tear it down.
  g_ota->setMdnsEnabled(false);

  if (settingsAccessTokenSet()) {
    g_ota->setPassword(g_token);
  }

  g_ota->onStart([]() {
    g_lastPercent = -1;
    stopAllWavPlayback();

    if (g_ota->getCommand() != U_FLASH) {
      LittleFS.end();
    }

    serialLogPrintln(g_ota->getCommand() == U_FLASH ? "OTA: firmware" : "OTA: filesystem");
    showOledText("OTA UPDATE", "0%");
  });

  g_ota->onProgress(showOtaProgress);

  g_ota->onEnd([]() {
    serialLogPrintln("OTA: done, rebooting");
    showOledText("OTA UPDATE", "Rebooting");
  });

  g_ota->onError([](ota_error_t error) {
    serialLogPrint("OTA error: ");
    serialLogPrintln((int)error);

    if (error == OTA_AUTH_ERROR) {
      return;
    }

    if (g_ota->getCommand() == U_FLASH) {
      showOledText("OTA FAILED");
      return;
    }

    // LittleFS is unmounted and possibly half-written; a reboot remounts or reports it.
    showOledText("OTA FAILED", "Rebooting");
    delay(2000);
    ESP.restart();
  });

  g_ota->begin();
  confirmRunningFirmware();
  serialLogPrintln(settingsAccessTokenSet() ? "OTA ready (password)" : "OTA ready");
}

void pollOta() {
  if (!g_ota) {
    return;
  }

  if (strcmp(g_token, settingsAccessToken()) != 0) {
    g_ota.reset();
    startOta();
  }

  g_ota->handle();
}

#endif  // TE_OTA
