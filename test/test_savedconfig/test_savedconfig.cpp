#include <chrono>
#include <string>
#include <functional>
#include <algorithm>

#include <Arduino.h>
#include <unity.h>
#include "SavedConfig.hpp"
#include <AuthProvider.hpp>
#include <FabBackend.hpp>
#include "BoardLogic.hpp"
#include "BufferedMsg.hpp"

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
    for (auto i = 0; i < defaults.cachedRfid.size(); i++)
    {
      const auto &tag = defaults.cachedRfid[i];
      TEST_ASSERT_TRUE_MESSAGE(tag.uid == 0, "Default config cachedRfid not empty");
      TEST_ASSERT_TRUE_MESSAGE(tag.level == FabUser::UserLevel::Unknown, "Default config cachedRfid not empty");
    }

    AuthProvider authProvider(secrets::cards::whitelist);
    TEST_ASSERT_TRUE_MESSAGE(authProvider.saveCache(), "AuthProvider saveCache failed");

    defaults = SavedConfig::LoadFromEEPROM().value_or(SavedConfig::DefaultConfig());

    // Test that default config is still empty
    for (auto i = 0; i < defaults.cachedRfid.size(); i++)
    {
      const auto &tag = defaults.cachedRfid[i];
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

  bool test_deserialization(Buffer &buff)
  {
    JsonDocument json_doc;
    buff.toJson(json_doc, "message_buffer");
    std::string json_text;
    auto chars = serializeJson(json_doc, json_text);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, chars, "Non empty JSON text");

    ESP_LOGD(TAG, "test_deserialization: json_text:%s", json_text.c_str());

    auto result = Buffer::fromJsonElement(json_doc["message_buffer"]);
    TEST_ASSERT_TRUE_MESSAGE(result.has_value(), "Deserialization succeeds");

    auto &new_buff = result.value();

    TEST_ASSERT_EQUAL_MESSAGE(buff.count(), new_buff.count(), "Messages are lost in json");

    while (new_buff.count() > 0)
    {
      const auto &msg_new = new_buff.getMessage();
      const auto &msg_old = buff.getMessage();
      TEST_ASSERT_EQUAL_STRING_MESSAGE(msg_new.mqtt_message.c_str(), msg_old.mqtt_message.c_str(), "Messages are in different order or damaged");
      TEST_ASSERT_EQUAL_STRING_MESSAGE(msg_new.mqtt_topic.c_str(), msg_old.mqtt_topic.c_str(), "Topics are in different order or damaged");
      TEST_ASSERT_EQUAL_MESSAGE(msg_new.wait_for_answer, msg_old.wait_for_answer, "Wait flags are in different order or damaged");
    }
    return true;
  }

  void test_buffered_msg()
  {
    constexpr auto NUM_MESSAGES = std::min(30, Buffer::MAX_MESSAGES - 3);

    Buffer buff;
    BufferedMsg msg1{"msg1", "topic1", false};
    BufferedMsg msg2{"msg2", "topic1", false};
    BufferedMsg msg3{"msg3", "topic2", false};
    std::vector messages{msg3, msg2, msg1};

    TEST_ASSERT_TRUE(msg1.mqtt_message == "msg1");
    TEST_ASSERT_TRUE(msg1.mqtt_topic == "topic1");
    TEST_ASSERT_TRUE(msg2.mqtt_message == "msg2");
    TEST_ASSERT_TRUE(msg2.mqtt_topic == "topic1");
    TEST_ASSERT_TRUE(msg3.mqtt_message == "msg3");
    TEST_ASSERT_TRUE(msg3.mqtt_topic == "topic2");

    // Create some more messages
    for (auto msg_num = 0; msg_num < NUM_MESSAGES; msg_num++)
    {
      std::string message{"{action=\"test\", value=\""};
      message.append(std::to_string(msg_num));
      message.append("\", test: true}");
      std::string topic{"machine/TESTTOPIC/"};

      topic.append(std::to_string(msg_num));
      messages.push_back({message, topic, true});
    }

    TEST_ASSERT_TRUE_MESSAGE(buff.hasChanged(), "Buffer has pending changes");

    // Nothing to test
    if constexpr (!conf::debug::ENABLE_BUFFERING)
      return;

    // Test insertion, messages must be queued newest first (msg3-msg2-msg1)
    auto msg_count = 0;
    for (const auto &msg : messages)
    {
      TEST_ASSERT_EQUAL_MESSAGE(msg_count, buff.count(), "Push_back: Buffer count is correct");
      const auto &bmsg = BufferedMsg{msg.mqtt_message, msg.mqtt_topic, msg.wait_for_answer};
      buff.push_front(bmsg);
      msg_count++;
    }

    TEST_ASSERT_EQUAL_MESSAGE(messages.size(), buff.count(), "Buffer contains all expected messages");

    TEST_ASSERT_TRUE_MESSAGE(buff.hasChanged(), "Buffer has pending changes");
    buff.setChanged(false);

    // Test retrieval oldest first (msg1-msg2-msg3-...)
    for (auto elem = messages.rbegin(); elem != messages.rend(); ++elem)
    {
      auto &msg = buff.getMessage();
      TEST_ASSERT_EQUAL_STRING_MESSAGE(elem->mqtt_message.c_str(), msg.mqtt_message.c_str(), "(1) Retrieval oldest first message is correct");
      TEST_ASSERT_EQUAL_STRING_MESSAGE(elem->mqtt_topic.c_str(), msg.mqtt_topic.c_str(), "(1) Retrieval oldest first topic is correct");
      TEST_ASSERT_EQUAL_MESSAGE(elem->wait_for_answer, msg.wait_for_answer, "(1) Retrieval oldest first wait_for_answer is correct");
    }

    TEST_ASSERT_EQUAL_MESSAGE(0, buff.count(), "Buffer is now empty");
    TEST_ASSERT_TRUE_MESSAGE(buff.hasChanged(), "Buffer has pending changes");
    buff.setChanged(false);

    // Test insertion in reverse order
    msg_count = 0;
    for (const auto &msg : messages)
    {
      TEST_ASSERT_EQUAL_MESSAGE(msg_count, buff.count(), "(1) Push_front: Buffer count is correct");
      const auto &bmsg = BufferedMsg{msg.mqtt_message, msg.mqtt_topic, msg.wait_for_answer};
      buff.push_back(bmsg);
      msg_count++;
    }

    TEST_ASSERT_TRUE_MESSAGE(buff.hasChanged(), "Buffer has pending changes");
    buff.setChanged(false);

    for (auto elem = messages.begin(); elem != messages.end(); ++elem)
    {
      auto &msg = buff.getMessage();
      TEST_ASSERT_EQUAL_STRING_MESSAGE(elem->mqtt_message.c_str(), msg.mqtt_message.c_str(), "(2) Retrieval oldest first message is correct");
      TEST_ASSERT_EQUAL_STRING_MESSAGE(elem->mqtt_topic.c_str(), msg.mqtt_topic.c_str(), "(2) Retrieval oldest first topic is correct");
      TEST_ASSERT_EQUAL_MESSAGE(elem->wait_for_answer, msg.wait_for_answer, "(2) Retrieval oldest first wait_for_answer is correct");
    }

    TEST_ASSERT_EQUAL_MESSAGE(0, buff.count(), "(2) Buffer is now empty");
    TEST_ASSERT_TRUE_MESSAGE(buff.hasChanged(), "Buffer has pending changes");
    buff.setChanged(false);

    // Test empty buffer
    TEST_ASSERT_TRUE_MESSAGE(test_deserialization(buff), "JSON works for empty buffer");
    TEST_ASSERT_FALSE_MESSAGE(buff.hasChanged(), "Buffer has no pending changes");

    // Insert more messages
    msg_count = 0;
    for (const auto &msg : messages)
    {
      TEST_ASSERT_EQUAL_MESSAGE(msg_count, buff.count(), "(2) Push_front: Buffer count is correct");
      const auto &bmsg = BufferedMsg{msg.mqtt_message, msg.mqtt_topic, msg.wait_for_answer};
      buff.push_back(bmsg);
      msg_count++;
    }

    TEST_ASSERT_TRUE_MESSAGE(test_deserialization(buff), "JSON works for full buffer");
    TEST_ASSERT_TRUE_MESSAGE(buff.hasChanged(), "Buffer has pending changes");
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
  esp_log_level_set(TAG, LOG_LOCAL_LEVEL);
  
  auto original = fabomatic::SavedConfig::LoadFromEEPROM();

  UNITY_BEGIN();
  RUN_TEST(fabomatic::tests::test_defaults);
  RUN_TEST(fabomatic::tests::test_changes);
  RUN_TEST(fabomatic::tests::test_magic_number);
  RUN_TEST(fabomatic::tests::test_rfid_cache);
  RUN_TEST(fabomatic::tests::test_buffered_msg);

  if (original.has_value())
  {
    original.value().SaveToEEPROM();
  }

  UNITY_END(); // stop unit testing
}

void loop()
{
}
