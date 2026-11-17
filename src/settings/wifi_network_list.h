#pragma once

#include <cstddef>

#include "settings/settings.h"

struct WifiNetwork {
  char ssid[SETTINGS_WIFI_SSID_MAX_LEN + 1];
  char password[SETTINGS_WIFI_PASSWORD_MAX_LEN + 1];
};

// Saved networks; boot tries the ones a scan misses in this order.
class WifiNetworkList {
public:
  size_t count() const;
  const WifiNetwork& at(size_t index) const;
  const WifiNetwork* data() const;
  int find(const char* ssid) const;
  const char* passwordFor(const char* ssid) const;

  // Appends the network. False when the credentials are invalid, the SSID
  // is already listed, or the list is full.
  bool add(const char* ssid, const char* password);

  // NVS blob: count() entries back to back. False (list unchanged) when the
  // blob is not a whole number of valid entries.
  bool load(const void* blob, size_t length);
  void clear();

private:
  WifiNetwork networks_[SETTINGS_WIFI_NETWORK_MAX] = {};
  size_t count_ = 0;
};
