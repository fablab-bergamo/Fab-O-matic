#include <atomic>
#include <chrono>
#include <functional>
#include <pthread.h>
#include <string>
#include <vector>

#include "BoardLogic.hpp"
#include "FabBackend.hpp"
#include "LCDWrapper.hpp"
#include "RFIDWrapper.hpp"
#include "SavedConfig.hpp"
#include "Tasks.hpp"
#include "conf.hpp"
#include "mock/MockMQTTBroker.hpp"
#include "mock/MockMrfc522.hpp"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <unity.h>
#include "LiquidCrystal.h"

using namespace std::chrono_literals;

pthread_t thread_mqtt_broker;
pthread_attr_t attr_mqtt_broker;

[[maybe_unused]] static const char *TAG3 = "test_mqtt";

namespace fabomatic::tests
{
  fabomatic::RFIDWrapper<fabomatic::MockMrfc522> rfid;
  fabomatic::LCDWrapper<LiquidCrystal> lcd{fabomatic::pins.lcd};
  fabomatic::BoardLogic logic;
  fabomatic::MockMQTTBroker broker;
  fabomatic::Tasks::Scheduler test_scheduler;

  std::atomic<bool> exit_request{false};

  void *threadMQTTServer(void *arg)
  {
    while (!exit_request)
    {
      broker.mainLoop();
      delay(15);
    }
    ESP_LOGI(TAG3, "MQTT server thread exiting");
    return arg;
  }

  void test_create_buffered_messages()
  {
    auto &server = logic.getServer();
    for (const auto &[uid, level, name] : secrets::cards::whitelist)
    {
      auto result = server.startUse(uid);
      TEST_ASSERT_FALSE_MESSAGE(result->request_ok, "(1) Request should have failed");

      result = server.inUse(uid, 1s);
      TEST_ASSERT_FALSE_MESSAGE(result->request_ok, "(2) Request should have failed");

      result = server.finishUse(uid, 2s);
      TEST_ASSERT_FALSE_MESSAGE(result->request_ok, "(3) Request should have failed");

      result = server.startUse(uid);
      TEST_ASSERT_FALSE_MESSAGE(result->request_ok, "(4) Request should have failed");

      result = server.finishUse(uid, 3s);
      TEST_ASSERT_FALSE_MESSAGE(result->request_ok, "(5) Request should have failed");

      result = server.registerMaintenance(uid);
      TEST_ASSERT_FALSE_MESSAGE(result->request_ok, "(6) Request should have failed");
    }
    // Should have generated 5 * 10 = 50 messages, truncated to 40.

    TEST_ASSERT_TRUE_MESSAGE(server.hasBufferedMsg(), "There are pending messages");
    TEST_ASSERT_TRUE_MESSAGE(server.saveBuffer(), "Saving pending messages works");
  }

  void test_check_transmission()
  {
    auto &server = logic.getServer();

    TEST_ASSERT_TRUE_MESSAGE(server.hasBufferedMsg(), "(1) There are pending messages");

    TEST_ASSERT_TRUE_MESSAGE(server.connect(), "Server connect works");

    TEST_ASSERT_TRUE_MESSAGE(server.hasBufferedMsg(), "(2) There are pending messages");

    TEST_ASSERT_TRUE_MESSAGE(server.alive(), "Alive request works");

    // Since old messages must be sent first, the buffer shall be empty now
    TEST_ASSERT_FALSE_MESSAGE(server.hasBufferedMsg(), "There are no more pending messages");

    TEST_ASSERT_TRUE_MESSAGE(server.saveBuffer(), "Saving pending messages works");
  }

  void test_start_broker()
  {
    auto &server = logic.getServer();
    server.setChannel(-1);
    TEST_ASSERT_TRUE_MESSAGE(server.connectWiFi(), "WiFi works");

    // Set the same IP Adress as MQTT server
    auto config = SavedConfig::LoadFromEEPROM();
    TEST_ASSERT_TRUE_MESSAGE(config.has_value(), "Config load failed");
    config.value().mqtt_server.assign("127.0.0.1");
    TEST_ASSERT_TRUE_MESSAGE(config.value().SaveToEEPROM(), "Config save failed");
    server.configure(config.value());

    // Start MQTT server thread in simulation
    attr_mqtt_broker.stacksize = 3 * 1024; // Required for ESP32-S2
    attr_mqtt_broker.detachstate = PTHREAD_CREATE_DETACHED;
    exit_request = false;
    pthread_create(&thread_mqtt_broker, &attr_mqtt_broker, threadMQTTServer, NULL);

    auto start = fabomatic::Tasks::arduinoNow();
    constexpr auto timeout = 5s;
    while (!broker.isRunning() && fabomatic::Tasks::arduinoNow() - start < timeout)
    {
      delay(100);
    }
    TEST_ASSERT_TRUE_MESSAGE(broker.isRunning(), "MQTT server not running");
    delay(5000);
    TEST_ASSERT_TRUE_MESSAGE(server.connect(), "Server connect failed in start MQTT");
  }

  void test_stop_broker()
  {
    exit_request = true;
    pthread_join(thread_mqtt_broker, NULL);
  }

  void test_fabserver_calls()
  {
    const int NB_TESTS = 10;
    const int NB_MACHINES = 5;
    auto &server = logic.getServer();
    for (uint16_t mid = 100; mid <= 100 + NB_MACHINES; mid++)
    {
      // Change MachineID on the fly
      auto saved_config = SavedConfig::DefaultConfig();

      saved_config.setMachineID(mid);
      saved_config.mqtt_server.assign("127.0.0.1");

      ESP_LOGI(TAG3, "Testing machine %d", mid);
      server.configure(saved_config);

      auto connected = false;
      for (auto i = 0; i < 3; i++)
      {
        server.disconnect(); // Force disconnect
        if (connected |= server.connect())
        {
          break;
        }
      }

      TEST_ASSERT_TRUE_MESSAGE(connected, "Server connect failed");
      TEST_ASSERT_TRUE_MESSAGE(server.isOnline(), "Server is not online");

      for (auto i = 0; i < NB_TESTS; ++i)
      {
        server.loop();
        card::uid_t uid = 123456789 + i;
        auto response = server.checkCard(uid);
        TEST_ASSERT_TRUE_MESSAGE(response != nullptr, "Server checkCard failed");
        ESP_LOGD(TAG3, "Server checkCard response: %s", response->toString().c_str());
        TEST_ASSERT_TRUE_MESSAGE(response->request_ok, "Server checkCard request failed");

        auto machine_resp = server.checkMachine(); // Machine ID is in the topic already
        TEST_ASSERT_TRUE_MESSAGE(machine_resp != nullptr, "Server checkMachine failed");
        TEST_ASSERT_TRUE_MESSAGE(machine_resp->request_ok, "Server checkMachine request failed");
        auto maintenance_resp = server.registerMaintenance(uid);
        TEST_ASSERT_TRUE_MESSAGE(maintenance_resp != nullptr, "Server registerMaintenance failed");
        TEST_ASSERT_TRUE_MESSAGE(maintenance_resp->request_ok, "Server registerMaintenance request failed");
        auto start_use_resp = server.startUse(uid);
        TEST_ASSERT_TRUE_MESSAGE(start_use_resp != nullptr, "Server startUse failed");
        TEST_ASSERT_TRUE_MESSAGE(start_use_resp->request_ok, "Server startUse request failed");
        auto in_use_resp = server.inUse(uid, 5s);
        TEST_ASSERT_TRUE_MESSAGE(in_use_resp != nullptr, "Server inUse failed");
        TEST_ASSERT_TRUE_MESSAGE(in_use_resp->request_ok, "Server inUse request failed");
        auto stop_use_resp = server.finishUse(uid, 10s);
        TEST_ASSERT_TRUE_MESSAGE(stop_use_resp != nullptr, "Server stopUse failed");
        TEST_ASSERT_TRUE_MESSAGE(stop_use_resp->request_ok, "Server stopUse request failed");
        auto alive_resp = server.alive();
        TEST_ASSERT_TRUE_MESSAGE(alive_resp, "Server alive failed");
      }
    }
    // Test authentication in online scenario
    AuthProvider auth(secrets::cards::whitelist);
    TEST_ASSERT_TRUE_MESSAGE(server.connect(), "Server is online");
    for (const auto &[uid, level, name] : secrets::cards::whitelist)
    {
      server.loop();
      auto response = auth.tryLogin(uid, server);
      TEST_ASSERT_TRUE_MESSAGE(response.has_value(), "Server checkCard failed");
      if (level == FabUser::UserLevel::Unknown)
      {
        TEST_ASSERT_FALSE_MESSAGE(response.value().authenticated, "Server returned authenticated user for invalid one");
      }
      else
      {
        TEST_ASSERT_TRUE_MESSAGE(response.value().authenticated, "Server returned unauthenticated user for a valid one");
        TEST_ASSERT_TRUE_MESSAGE(response.value().user_level == level, "Server returned wrong user level");
      }
    }
    // Test that saving the cache works
    TEST_ASSERT_TRUE_MESSAGE(auth.saveCache(), "AuthProvider saveCache failed");

    // Test that the cache contains all the valid whitelist entries
    auto cached_entries = SavedConfig::LoadFromEEPROM().value().cachedRfid;
    for (const auto &[uid, level, name] : secrets::cards::whitelist)
    {
      const auto &cached_card = cached_entries.find_uid(uid);
      TEST_ASSERT_TRUE_MESSAGE(cached_card || level == FabUser::UserLevel::Unknown,
                               "AuthProvider saveCache failed to save all whitelist entries");
    }
  }

  /// @brief Opens WiFi and server connection and updates board state accordingly
  void test_taskConnect()
  {
    auto &server = logic.getServer();
    if (!server.isOnline())
    {
      // connection to wifi
      logic.changeStatus(BoardLogic::Status::Connecting);

      // Try to connect
      server.connect();
      // Refresh after connection
      logic.changeStatus(server.isOnline() ? BoardLogic::Status::Connected : BoardLogic::Status::Offline);
    }

    if (server.isOnline())
    {
      ESP_LOGI(TAG3, "taskConnect - online, calling refreshFromServer");
      // Get machine data from the server if it is online
      logic.refreshFromServer();
    }
  }

  /// @brief periodic check for new RFID card
  void test_taskCheckRfid()
  {
    logic.checkRfid();
  }

  /// @brief blink led
  void test_taskVarious()
  {
    logic.blinkLed();
    logic.checkPowerOff();

    if (logic.getMachine().isShutdownImminent())
    {
      logic.beepFail();
      ESP_LOGI(TAG3, "Machine is about to shutdown");
    }

    // auto logout after delay
    auto &machine = logic.getMachine();
    if (machine.isAutologoffExpired())
    {
      ESP_LOGI(TAG3, "Auto-logging out user %s\r\n", machine.getActiveUser().holder_name.data());
      logic.logout();
      logic.beepFail();
    }
  }

  void test_taskEspWatchdog()
  {
    static auto initialized = false;

    if (conf::tasks::WATCHDOG_TIMEOUT > 0s)
    {
      if (!initialized)
      {
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(conf::tasks::WATCHDOG_TIMEOUT).count();
        TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, esp_task_wdt_init(secs, true), "taskEspWatchdog - esp_task_wdt_init failed");
        ESP_LOGI(TAG3, "taskEspWatchdog - initialized %d seconds", secs);
        TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, esp_task_wdt_add(NULL), "taskEspWatchdog - esp_task_wdt_add failed");
        initialized = true;
      }
      TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, esp_task_wdt_reset(), "taskEspWatchdog - esp_task_wdt_reset failed");
    }
  }

  /// @brief checks the RFID chip status and re-init it if necessary.
  void test_taskRfidWatchdog()
  {
    auto result = rfid.selfTest();
    TEST_ASSERT_TRUE_MESSAGE(result, "test_taskRfidWatchdog: RFID chip failure");
    if (!result)
    {
      ESP_LOGE(TAG3, "RFID chip failure");

      // Infinite retry until success or hw watchdog timeout
      while (!rfid.rfidInit())
        TEST_FAIL_MESSAGE("Init RFID chip failed");
    }
  }

  /// @brief sends the MQTT alive message
  void test_taskMQTTAlive()
  {
    auto &server = logic.getServer();
    if (server.isOnline())
    {
      TEST_ASSERT_TRUE_MESSAGE(server.loop(), "test_taskMQTTAlive: Server loop failed");
    }
  }

  auto t1 = Tasks::Task("taskConnect", 15s, &test_taskConnect, test_scheduler);
  auto t2 = Tasks::Task("taskCheckRfid", 50ms, &test_taskCheckRfid, test_scheduler);
  auto t3 = Tasks::Task("taskVarious", 1s, &test_taskVarious, test_scheduler);
  auto t4 = Tasks::Task("taskEspWatchdog", 5s, &test_taskEspWatchdog, test_scheduler);
  auto t5 = Tasks::Task("taskRfidWatchdog", 30s, &test_taskRfidWatchdog, test_scheduler);
  auto t6 = Tasks::Task("taskMQTTAlive", 30s, &test_taskMQTTAlive, test_scheduler);

  void test_normal_use()
  {
    test_scheduler.updateSchedules();
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(6, test_scheduler.taskCount(), "Scheduler does not contain all tasks");
    auto start = fabomatic::Tasks::arduinoNow();
    while (fabomatic::Tasks::arduinoNow() - start <= 1min)
    {
      test_scheduler.execute();
      delay(25);
    }
    // Check that all tasks ran at least once
    for (const auto tp : test_scheduler.getTasks())
    {
      const auto t = *tp;
      ESP_LOGD(TAG3, "Task %s: %lu runs, %lu ms total runtime, %lu ms avg tardiness", t.getId().c_str(), t.getRunCounter(), t.getTotalRuntime().count(), t.getAvgTardiness().count());
      TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(1, t.getRunCounter(), "Task did not run");
    }
    // Remove the HW Watchdog
    esp_task_wdt_delete(NULL);
  }
} // namespace fabomatic::Tests

void tearDown(void) {};

void setUp(void)
{
  TEST_ASSERT_TRUE_MESSAGE(fabomatic::tests::logic.configure(fabomatic::tests::rfid, fabomatic::tests::lcd), "BoardLogic configure failed");
  TEST_ASSERT_TRUE_MESSAGE(fabomatic::tests::logic.initBoard(), "BoardLogic init failed");
};

void setup()
{
  delay(1000);
  // Save original config
  auto original = fabomatic::SavedConfig::LoadFromEEPROM();

  UNITY_BEGIN();
  RUN_TEST(fabomatic::tests::test_create_buffered_messages);
  RUN_TEST(fabomatic::tests::test_start_broker);
  RUN_TEST(fabomatic::tests::test_check_transmission);
  RUN_TEST(fabomatic::tests::test_fabserver_calls);
  RUN_TEST(fabomatic::tests::test_normal_use);
  RUN_TEST(fabomatic::tests::test_stop_broker);

  UNITY_END(); // stop unit testing

  // Restore original config
  if (original.has_value())
  {
    original.value().SaveToEEPROM();
  }
};

void loop()
{
}