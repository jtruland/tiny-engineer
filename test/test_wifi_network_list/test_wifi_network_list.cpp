#include <cstring>
#include <unity.h>

#include "settings/wifi_network_list.h"

void test_add_keeps_order() {
  WifiNetworkList networks;

  TEST_ASSERT_TRUE(networks.add("home", "secret"));
  TEST_ASSERT_TRUE(networks.add("office", ""));

  TEST_ASSERT_EQUAL(2, networks.count());
  TEST_ASSERT_EQUAL_STRING("home", networks.at(0).ssid);
  TEST_ASSERT_EQUAL_STRING("secret", networks.at(0).password);
  TEST_ASSERT_EQUAL_STRING("office", networks.at(1).ssid);
  TEST_ASSERT_EQUAL(1, networks.find("office"));
  TEST_ASSERT_EQUAL(-1, networks.find("cafe"));
  TEST_ASSERT_EQUAL_STRING("secret", networks.passwordFor("home"));
  TEST_ASSERT_NULL(networks.passwordFor("cafe"));
}

void test_add_rejects_duplicates() {
  WifiNetworkList networks;
  networks.add("home", "old");

  TEST_ASSERT_FALSE(networks.add("home", "new"));
  TEST_ASSERT_EQUAL(1, networks.count());
  TEST_ASSERT_EQUAL_STRING("old", networks.at(0).password);
}

void test_add_rejects_a_sixth_network() {
  WifiNetworkList networks;
  const char* ssids[] = {"a", "b", "c", "d", "e"};

  for (const char* ssid : ssids) {
    TEST_ASSERT_TRUE(networks.add(ssid, ""));
  }

  TEST_ASSERT_FALSE(networks.add("f", ""));
  TEST_ASSERT_EQUAL(SETTINGS_WIFI_NETWORK_MAX, networks.count());
}

void test_add_rejects_invalid_credentials() {
  WifiNetworkList networks;
  char longPassword[SETTINGS_WIFI_PASSWORD_MAX_LEN + 2] = {};
  memset(longPassword, 'p', SETTINGS_WIFI_PASSWORD_MAX_LEN + 1);

  TEST_ASSERT_FALSE(networks.add("", "secret"));
  TEST_ASSERT_FALSE(networks.add("home", longPassword));
  TEST_ASSERT_FALSE(networks.add("home", nullptr));
  TEST_ASSERT_EQUAL(0, networks.count());
}

void test_load_round_trips_saved_blob() {
  WifiNetworkList saved;
  saved.add("home", "secret");
  saved.add("office", "");

  WifiNetworkList loaded;

  TEST_ASSERT_TRUE(
    loaded.load(saved.data(), saved.count() * sizeof(WifiNetwork))
  );
  TEST_ASSERT_EQUAL(2, loaded.count());
  TEST_ASSERT_EQUAL_STRING("home", loaded.at(0).ssid);
  TEST_ASSERT_EQUAL_STRING("secret", loaded.at(0).password);
}

void test_load_rejects_corrupt_blob() {
  WifiNetworkList networks;
  networks.add("home", "secret");

  WifiNetwork corrupt[2] = {};
  memset(corrupt[1].ssid, 'x', sizeof(corrupt[1].ssid));
  strcpy(corrupt[0].ssid, "ok");

  TEST_ASSERT_FALSE(networks.load(corrupt, sizeof(WifiNetwork) + 1));
  TEST_ASSERT_FALSE(networks.load(corrupt, sizeof(corrupt)));

  WifiNetwork emptySsid[1] = {};
  TEST_ASSERT_FALSE(networks.load(emptySsid, sizeof(emptySsid)));

  TEST_ASSERT_EQUAL(1, networks.count());
  TEST_ASSERT_EQUAL_STRING("home", networks.at(0).ssid);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_add_keeps_order);
  RUN_TEST(test_add_rejects_duplicates);
  RUN_TEST(test_add_rejects_a_sixth_network);
  RUN_TEST(test_add_rejects_invalid_credentials);
  RUN_TEST(test_load_round_trips_saved_blob);
  RUN_TEST(test_load_rejects_corrupt_blob);
  return UNITY_END();
}
