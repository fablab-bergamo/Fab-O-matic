#include <chrono>
#include <string>
#include <functional>

#include <Arduino.h>
#include <unity.h>
#include "SavedConfig.hpp"

using namespace std::chrono_literals;

namespace fablabbg::tests
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
    TEST_ASSERT_EQUAL_STRING_MESSAGE(defaults.ssid, loaded.ssid, "Loaded config ssid mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(defaults.password, loaded.password, "Loaded config password mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(defaults.mqtt_server, loaded.mqtt_server, "Loaded config mqtt_server mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(defaults.mqtt_user, loaded.mqtt_user, "Loaded config mqtt_user mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(defaults.mqtt_password, loaded.mqtt_password, "Loaded config mqtt_password mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(defaults.machine_topic, loaded.machine_topic, "Loaded config machine_topic mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(defaults.machine_id, loaded.machine_id, "Loaded config machine_id mismatch");
    TEST_ASSERT_TRUE_MESSAGE(SavedConfig::MAGIC_NUMBER == loaded.magic_number, "Loaded config magic number mismatch");
  }

  void test_changes(void)
  {
    auto result1 = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(result1.has_value(), "Loaded config is empty");
    auto result2 = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(result2.has_value(), "Loaded config is empty");

    auto original = result1.value();
    auto loaded = result2.value();

    loaded.ssid[0] = 'a';
    loaded.password[0] = 'b';
    loaded.mqtt_server[0] = 'c';
    loaded.mqtt_user[0] = 'd';
    loaded.mqtt_password[0] = 'e';
    loaded.machine_topic[0] = 'f';
    loaded.machine_id[0] = '9';

    // Save changes
    TEST_ASSERT_TRUE_MESSAGE(loaded.SaveToEEPROM(), "Loaded config save failed");
    result2 = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(result2.has_value(), "Loaded config is empty");
    auto saved = result2.value();

    TEST_ASSERT_EQUAL_STRING_MESSAGE(loaded.ssid, saved.ssid, "Loaded config ssid mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(loaded.password, saved.password, "Loaded config password mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(loaded.mqtt_server, saved.mqtt_server, "Loaded config mqtt_server mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(loaded.mqtt_user, saved.mqtt_user, "Loaded config mqtt_user mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(loaded.mqtt_password, saved.mqtt_password, "Loaded config mqtt_password mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(loaded.machine_topic, saved.machine_topic, "Loaded config machine_topic mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(loaded.machine_id, saved.machine_id, "Loaded config machine_id mismatch");
    TEST_ASSERT_TRUE_MESSAGE(SavedConfig::MAGIC_NUMBER == saved.magic_number, "Loaded config magic number mismatch");

    // Check that changes have been saved
    TEST_ASSERT_FALSE_MESSAGE(strcmp(original.ssid, saved.ssid) == 0, "Loaded config ssid mismatch");
    TEST_ASSERT_FALSE_MESSAGE(strcmp(original.password, saved.password) == 0, "Loaded config password mismatch");
    TEST_ASSERT_FALSE_MESSAGE(strcmp(original.mqtt_server, saved.mqtt_server) == 0, "Loaded config mqtt_server mismatch");
    TEST_ASSERT_FALSE_MESSAGE(strcmp(original.mqtt_user, saved.mqtt_user) == 0, "Loaded config mqtt_user mismatch");
    TEST_ASSERT_FALSE_MESSAGE(strcmp(original.mqtt_password, saved.mqtt_password) == 0, "Loaded config mqtt_password mismatch");
    TEST_ASSERT_FALSE_MESSAGE(strcmp(original.machine_topic, saved.machine_topic) == 0, "Loaded config machine_topic mismatch");
    TEST_ASSERT_FALSE_MESSAGE(strcmp(original.machine_id, saved.machine_id) == 0, "Loaded config machine_id mismatch");

    // Restore original
    TEST_ASSERT_TRUE_MESSAGE(original.SaveToEEPROM(), "Loaded config save failed");
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
} // namespace fablabbg::tests

void tearDown(void)
{
  fablabbg::tests::original.SaveToEEPROM();
}

void setUp(void)
{
  // set stuff up here
  auto result = fablabbg::SavedConfig::LoadFromEEPROM();
  if (result.has_value())
  {
    fablabbg::tests::original = result.value();
  }
  else
  {
    fablabbg::tests::original = fablabbg::SavedConfig::DefaultConfig();
  }
}

void setup()
{
  delay(1000);
  auto original = fablabbg::SavedConfig::LoadFromEEPROM();

  UNITY_BEGIN();
  RUN_TEST(fablabbg::tests::test_defaults);
  RUN_TEST(fablabbg::tests::test_changes);
  RUN_TEST(fablabbg::tests::test_magic_number);
  UNITY_END(); // stop unit testing

  if (original.has_value())
  {
    original.value().SaveToEEPROM();
  }
}

void loop()
{
}