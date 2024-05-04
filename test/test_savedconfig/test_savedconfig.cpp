#include <chrono>
#include <string>
#include <functional>

#include <Arduino.h>
#include <unity.h>
#include "SavedConfig.hpp"
#include <AuthProvider.hpp>
#include <FabBackend.hpp>
#include "BoardLogic.hpp"

using namespace std::chrono_literals;

[[maybe_unused]] static const char *TAG3 = "test_logic";

namespace fabomatic::tests
{
  SavedConfig original;

  void test_defaults(void)
  {
    SavedConfig config;
    auto defaults = SavedConfig::DefaultConfig();
    TEST_ASSERT_TRUE_MESSAGE(SavedConfig::MAGIC_NUMBER == defaults.magic_number, "Default config magic number mismatch");

    TEST_ASSERT_TRUE_MESSAGE(defaults.SaveToEEPROM(), "Default config save failed");

    auto result = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(result.has_value(), "Loaded config is empty");
    auto loaded = result.value();
    TEST_ASSERT_TRUE_MESSAGE(defaults.ssid == loaded.ssid, "Loaded config ssid mismatch");
    TEST_ASSERT_TRUE_MESSAGE(defaults.password == loaded.password, "Loaded config password mismatch");
    TEST_ASSERT_TRUE_MESSAGE(defaults.mqtt_server == loaded.mqtt_server, "Loaded config mqtt_server mismatch");
    TEST_ASSERT_TRUE_MESSAGE(defaults.mqtt_user == loaded.mqtt_user, "Loaded config mqtt_user mismatch");
    TEST_ASSERT_TRUE_MESSAGE(defaults.mqtt_password == loaded.mqtt_password, "Loaded config mqtt_password mismatch");
    TEST_ASSERT_TRUE_MESSAGE(defaults.mqtt_switch_topic == loaded.mqtt_switch_topic, "Loaded config mqtt_switch_topic mismatch");
    TEST_ASSERT_TRUE_MESSAGE(defaults.machine_id == loaded.machine_id, "Loaded config machine_id mismatch");
    TEST_ASSERT_TRUE_MESSAGE(SavedConfig::MAGIC_NUMBER == loaded.magic_number, "Loaded config magic number mismatch");

    // Test set and get methods
    auto id = MachineID(123);
    loaded.setMachineID(id);
    TEST_ASSERT_TRUE_MESSAGE(id == loaded.getMachineID(), "Loaded config machine_id mismatch");
  }

  void test_changes(void)
  {
    auto result1 = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(result1.has_value(), "Loaded config is empty");
    auto result2 = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(result2.has_value(), "Loaded config is empty");

    auto &original = result1.value();
    auto &loaded = result2.value();

    loaded.ssid = "a";
    loaded.password = "b";
    loaded.mqtt_server = "c";
    loaded.mqtt_user = "d";
    loaded.mqtt_password = "e";
    loaded.mqtt_switch_topic = "f";
    loaded.machine_id = "9";

    // Save changes
    TEST_ASSERT_TRUE_MESSAGE(loaded.SaveToEEPROM(), "Loaded config save failed");
    result2 = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(result2.has_value(), "Loaded config is empty");
    auto &saved = result2.value();
    TEST_ASSERT_TRUE_MESSAGE(loaded.ssid == saved.ssid, "Loaded config ssid mismatch");
    TEST_ASSERT_TRUE_MESSAGE(loaded.password == saved.password, "Loaded config password mismatch");
    TEST_ASSERT_TRUE_MESSAGE(loaded.mqtt_server == saved.mqtt_server, "Loaded config mqtt_server mismatch");
    TEST_ASSERT_TRUE_MESSAGE(loaded.mqtt_user == saved.mqtt_user, "Loaded config mqtt_user mismatch");
    TEST_ASSERT_TRUE_MESSAGE(loaded.mqtt_password == saved.mqtt_password, "Loaded config mqtt_password mismatch");
    TEST_ASSERT_TRUE_MESSAGE(loaded.mqtt_switch_topic == saved.mqtt_switch_topic, "Loaded config mqtt_switch_topic mismatch");
    TEST_ASSERT_TRUE_MESSAGE(loaded.machine_id == saved.machine_id, "Loaded config machine_id mismatch");
    TEST_ASSERT_TRUE_MESSAGE(loaded.disablePortal == saved.disablePortal, "Loaded config disablePortal mismatch");
    TEST_ASSERT_TRUE_MESSAGE(SavedConfig::MAGIC_NUMBER == saved.magic_number, "Loaded config magic number mismatch");

    // Check that changes have been saved
    TEST_ASSERT_TRUE_MESSAGE(original.ssid != saved.ssid, "Loaded config ssid mismatch");
    TEST_ASSERT_TRUE_MESSAGE(original.password != saved.password, "Loaded config password mismatch");
    TEST_ASSERT_TRUE_MESSAGE(original.mqtt_server != saved.mqtt_server, "Loaded config mqtt_server mismatch");
    TEST_ASSERT_TRUE_MESSAGE(original.mqtt_user != saved.mqtt_user, "Loaded config mqtt_user mismatch");
    TEST_ASSERT_TRUE_MESSAGE(original.mqtt_password != saved.mqtt_password, "Loaded config mqtt_password mismatch");
    TEST_ASSERT_TRUE_MESSAGE(original.mqtt_switch_topic != saved.mqtt_switch_topic, "Loaded config mqtt_switch_topic mismatch");
    TEST_ASSERT_TRUE_MESSAGE(original.machine_id != saved.machine_id, "Loaded config machine_id mismatch");

    // Restore original
    TEST_ASSERT_TRUE_MESSAGE(original.SaveToEEPROM(), "Loaded config save failed");
  }

  void test_rfid_cache()
  {
    auto defaults = SavedConfig::DefaultConfig();
    defaults.SaveToEEPROM();

    TEST_ASSERT_TRUE_MESSAGE(defaults.cachedRfid.size() == conf::rfid_tags::CACHE_LEN, "Default config cachedRfid size mismatch");

    // Test that default config has empty cache
    for (const auto &tag : defaults.cachedRfid)
    {
      TEST_ASSERT_TRUE_MESSAGE(tag.uid == 0, "Default config cachedRfid not empty");
      TEST_ASSERT_TRUE_MESSAGE(tag.level == FabUser::UserLevel::Unknown, "Default config cachedRfid not empty");
    }

    AuthProvider authProvider(secrets::cards::whitelist);
    TEST_ASSERT_TRUE_MESSAGE(authProvider.saveCache(), "AuthProvider saveCache failed");

    defaults = SavedConfig::LoadFromEEPROM().value_or(SavedConfig::DefaultConfig());

    // Test that default config is still empty
    for (const auto &tag : defaults.cachedRfid)
    {
      TEST_ASSERT_TRUE_MESSAGE(tag.uid == 0, "Default config cachedRfid not empty after AuthProvider saveCache");
      TEST_ASSERT_TRUE_MESSAGE(tag.level == FabUser::UserLevel::Unknown, "Default config cachedRfid not empty after AuthProvider saveCache");
    }

    FabBackend server;
    defaults.mqtt_server = "INVALID_SERVER"; // Must be invalid to test the cache
    server.configure(defaults);

    auto [wl_uid, wl_level, name] = secrets::cards::whitelist[0];
    auto result = authProvider.tryLogin(wl_uid, server);

    TEST_ASSERT_TRUE_MESSAGE(result.has_value(), "AuthProvider tryLogin failed");

    // Now save the positive result (should not be do anything, because offline)
    TEST_ASSERT_TRUE_MESSAGE(authProvider.saveCache(), "AuthProvider saveCache 2 failed");
    // Reload the cache
    defaults = SavedConfig::LoadFromEEPROM().value_or(SavedConfig::DefaultConfig());

    TEST_ASSERT_TRUE_MESSAGE(0 == defaults.cachedRfid[0].uid, "AuthProvider tryLogin card_uid mismatch");
    TEST_ASSERT_TRUE_MESSAGE(FabUser::UserLevel::Unknown == defaults.cachedRfid[0].level, "AuthProvider tryLogin user_level mismatch");

    // Generate many events
    for (auto i = 0; i < 50; i++)
    {
      auto rnd = random(0, conf::rfid_tags::CACHE_LEN);
      auto result = authProvider.tryLogin(defaults.cachedRfid[rnd].uid, server);
    }

    TEST_ASSERT_TRUE_MESSAGE(authProvider.saveCache(), "AuthProvider saveCache 2 failed");
    defaults = SavedConfig::LoadFromEEPROM().value_or(SavedConfig::DefaultConfig());

    // Online scenario with cache is testing in MQTT testcase with the broker.
  }

  void test_magic_number()
  {
    auto result1 = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(result1.has_value(), "Loaded config is empty");
    auto loaded = result1.value();
    auto original_magic = loaded.magic_number;
    loaded.magic_number++;
    TEST_ASSERT_TRUE_MESSAGE(loaded.SaveToEEPROM(), "Loaded config save failed");
    TEST_ASSERT_EQUAL_MESSAGE(original_magic, loaded.magic_number, "Magic number overwritten by application");

    // Force abnormal case after FW update
    TEST_ASSERT_TRUE(EEPROM.begin(sizeof(SavedConfig)));
    loaded.magic_number = SavedConfig::MAGIC_NUMBER - 1;
    EEPROM.put(0, &loaded);
    TEST_ASSERT_TRUE(EEPROM.commit());

    // Now check the loaded version is null due to version mismatch
    auto result2 = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_FALSE_MESSAGE(result2.has_value(), "Loaded config is not empty");
  }
} // namespace fabomatic::tests

void tearDown(void)
{
  // clean stuff up here
}

void setUp(void)
{
  // set stuff up here
}

void setup()
{
  delay(1000);
  auto original = fabomatic::SavedConfig::LoadFromEEPROM();

  UNITY_BEGIN();
  RUN_TEST(fabomatic::tests::test_defaults);
  RUN_TEST(fabomatic::tests::test_changes);
  RUN_TEST(fabomatic::tests::test_magic_number);
  RUN_TEST(fabomatic::tests::test_rfid_cache);

  if (original.has_value())
  {
    original.value().SaveToEEPROM();
  }

  UNITY_END(); // stop unit testing
}

void loop()
{
}