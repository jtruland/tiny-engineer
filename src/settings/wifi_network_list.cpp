#include "settings/wifi_network_list.h"

#include <algorithm>
#include <cstring>

namespace {

bool terminated(const char* text, size_t size) {
  return memchr(text, '\0', size) != nullptr;
}

bool validEntry(const WifiNetwork& network) {
  return terminated(network.ssid, sizeof(network.ssid)) &&
         terminated(network.password, sizeof(network.password)) &&
         settingsValidateWifiSsid(network.ssid) &&
         settingsValidateWifiPassword(network.password);
}

}  // namespace

size_t WifiNetworkList::count() const {
  return count_;
}

const WifiNetwork& WifiNetworkList::at(size_t index) const {
  return networks_[index];
}

const WifiNetwork* WifiNetworkList::data() const {
  return networks_;
}

int WifiNetworkList::find(const char* ssid) const {
  const WifiNetwork* end = networks_ + count_;
  const WifiNetwork* match = std::find_if(
    networks_,
    end,
    [ssid](const WifiNetwork& network) {
      return strcmp(network.ssid, ssid) == 0;
    }
  );

  return match == end ? -1 : static_cast<int>(match - networks_);
}

const char* WifiNetworkList::passwordFor(const char* ssid) const {
  const int index = find(ssid);

  return index < 0 ? nullptr : networks_[index].password;
}

bool WifiNetworkList::add(const char* ssid, const char* password) {
  if (!settingsValidateWifiSsid(ssid) ||
      !settingsValidateWifiPassword(password) ||
      find(ssid) >= 0 ||
      count_ == SETTINGS_WIFI_NETWORK_MAX) {
    return false;
  }

  WifiNetwork& network = networks_[count_++];
  strncpy(network.ssid, ssid, SETTINGS_WIFI_SSID_MAX_LEN);
  strncpy(network.password, password, SETTINGS_WIFI_PASSWORD_MAX_LEN);
  return true;
}

bool WifiNetworkList::load(const void* blob, size_t length) {
  const size_t loadedCount = length / sizeof(WifiNetwork);

  if (length % sizeof(WifiNetwork) != 0 ||
      loadedCount > SETTINGS_WIFI_NETWORK_MAX) {
    return false;
  }

  const auto* loaded = static_cast<const WifiNetwork*>(blob);

  if (!std::all_of(loaded, loaded + loadedCount, validEntry)) {
    return false;
  }

  clear();
  std::copy(loaded, loaded + loadedCount, networks_);
  count_ = loadedCount;
  return true;
}

void WifiNetworkList::clear() {
  std::fill(networks_, networks_ + SETTINGS_WIFI_NETWORK_MAX, WifiNetwork{});
  count_ = 0;
}
