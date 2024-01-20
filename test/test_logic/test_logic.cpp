#include <chrono>
#include <string>
#include <functional>
#include <vector>
#include <atomic>
#include <pthread.h>

#include <Arduino.h>
#include <unity.h>
#include "FabServer.hpp"
#include "LCDWrapper.hpp"
#include "RFIDWrapper.hpp"
#include "mock/MockMrfc522.hpp"
#include "mock/MockLcdLibrary.hpp"
#include "BoardLogic.hpp"
#include "conf.hpp"
#include "SavedConfig.hpp"
#include "test_common.h"
#include "mock/MockMQTTBroker.hpp"

using namespace fablabbg;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace fablabbg::tests;

[[maybe_unused]] static const char *TAG3 = "test_logic";

RFIDWrapper<MockMrfc522> rfid;
LCDWrapper<MockLcdLibrary> lcd{pins.lcd};
BoardLogic logic;

constexpr card::uid_t get_test_uid(size_t idx)
{
  auto [card_uid, level, name] = test_whitelist[idx];
  return card_uid;
}

void test_machine_defaults()
{
  auto &machine = logic.getMachine();

  TEST_ASSERT_TRUE_MESSAGE(machine.isConfigured(), "Machine not configured");

  auto config_req = machine.getConfig();
  TEST_ASSERT_TRUE_MESSAGE(config_req.has_value(), "Machine config not available");

  auto config = config_req.value();

  auto result = machine.getMachineId().id == conf::default_config::machine_id.id;
  TEST_ASSERT_TRUE_MESSAGE(result, "Machine ID not per default configuration");

  result = machine.getMachineName() == conf::default_config::machine_name;
  TEST_ASSERT_TRUE_MESSAGE(result, "Machine Name not per default configuration");

  result = machine.getAutologoffDelay() == conf::machine::DEFAULT_AUTO_LOGOFF_DELAY;
  TEST_ASSERT_TRUE_MESSAGE(result, "Machine autologoff delay not per default configuration");

  result = machine.toString().length() > 0;
  TEST_ASSERT_TRUE_MESSAGE(result, "Machine toString() failed");

  TEST_ASSERT_TRUE_MESSAGE(config.hasRelay() || pins.relay.ch1_pin == NO_PIN, "Machine relay not configured");
  TEST_ASSERT_TRUE_MESSAGE(config.hasMqttSwitch() || conf::default_config::machine_topic.empty(), "Machine MQTT switch not configured");

  TEST_ASSERT_TRUE_MESSAGE(config.mqtt_config.topic == conf::default_config::machine_topic, "Machine MQTT topic not configured");
  TEST_ASSERT_TRUE_MESSAGE(config.relay_config.pin == pins.relay.ch1_pin, "Machine relay pin not configured");
  TEST_ASSERT_TRUE_MESSAGE(config.machine_id.id == conf::default_config::machine_id.id, "Machine ID not configured");
}

void test_simple_methods()
{
  logic.beep_failed();
  logic.beep_ok();
  logic.blinkLed();

  std::vector<BoardLogic::Status> statuses{BoardLogic::Status::ERROR_HW, BoardLogic::Status::ERROR,
                                           BoardLogic::Status::CONNECTED, BoardLogic::Status::CONNECTING,
                                           BoardLogic::Status::CLEAR, BoardLogic::Status::FREE, BoardLogic::Status::LOGGED_IN,
                                           BoardLogic::Status::LOGIN_DENIED, BoardLogic::Status::BUSY, BoardLogic::Status::LOGOUT,
                                           BoardLogic::Status::ALREADY_IN_USE, BoardLogic::Status::IN_USE, BoardLogic::Status::OFFLINE,
                                           BoardLogic::Status::NOT_ALLOWED, BoardLogic::Status::VERIFYING,
                                           BoardLogic::Status::MAINTENANCE_NEEDED, BoardLogic::Status::MAINTENANCE_QUERY,
                                           BoardLogic::Status::MAINTENANCE_DONE, BoardLogic::Status::PORTAL_STARTING,
                                           BoardLogic::Status::PORTAL_FAILED, BoardLogic::Status::PORTAL_OK};

  for (const auto status : statuses)
  {
    logic.changeStatus(status);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(status, logic.getStatus(), "Status mismatch");
  }

  for (auto i = 0; i < 3; ++i)
  {
    logic.checkPowerOff();
    logic.blinkLed();
    logic.refreshLCD();
    logic.updateLCD();
    logic.reconfigure();
  }
}

void test_whitelist_no_network()
{
  machine_init(logic, rfid);

  // Check whitelist is recognized
  for (auto &wle : test_whitelist)
  {
    const auto [card_uid, level, name] = wle;
    TEST_ASSERT_TRUE_MESSAGE(logic.authorize(card_uid), "Card not authorized");
    TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::LOGGED_IN, "Status not LOGGED_IN");
    TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getActiveUser().card_uid == card_uid, "User UID not equal");
    TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getActiveUser().holder_name == name, "User name not equal");
    TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getActiveUser().user_level == level, "User level not equal");
    logic.logout();
    TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::LOGOUT, "Status not LOGOUT");
  }

  // Check that machine is free
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::FREE, "Status not FREE");
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "Machine not free");

  // Logon simulating RFID tag
  const auto card_uid = get_test_uid(1);

  simulate_rfid_card(rfid, logic, card_uid, std::nullopt);
  TEST_ASSERT_TRUE_MESSAGE(rfid.isNewCardPresent(), "New card not present");
  TEST_ASSERT_TRUE_MESSAGE(rfid.readCardSerial().has_value(), "Card serial not read");
  TEST_ASSERT_TRUE_MESSAGE(rfid.readCardSerial().value() == card_uid, "Card serial not equal");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::LOGGED_IN, logic.getStatus(), "Status not LOGGED_IN");

  // Card away, machine shall be busy
  auto new_state = simulate_rfid_card(rfid, logic, std::nullopt);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::IN_USE, new_state, "Status not IN_USE");
  TEST_ASSERT_FALSE_MESSAGE(logic.getMachine().isFree(), "Machine is free");

  // Same card back, shall logout user
  new_state = simulate_rfid_card(rfid, logic, card_uid);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::LOGOUT, new_state, "Status not LOGOUT");
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "Machine is not free");
}

void test_one_user_at_a_time()
{
  machine_init(logic, rfid);

  const auto card_uid = get_test_uid(0);
  auto new_state = simulate_rfid_card(rfid, logic, card_uid);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::LOGGED_IN, new_state, "Status not LOGGED_IN");
  TEST_ASSERT_TRUE_MESSAGE(!logic.getMachine().isFree(), "Machine is free");

  // Card away, machine shall be busy
  new_state = simulate_rfid_card(rfid, logic, std::nullopt);
  TEST_ASSERT_TRUE_MESSAGE(!logic.getMachine().isFree(), "Machine is free");

  // New card, shall be denied
  const auto card_uid2 = get_test_uid(1);
  simulate_rfid_card(rfid, logic, card_uid2, 100ms);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getActiveUser().card_uid == card_uid, "User UID has changed");

  // New card away, first user shall still be here
  simulate_rfid_card(rfid, logic, std::nullopt);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getActiveUser().card_uid == card_uid, "User UID has changed");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::IN_USE, logic.getStatus(), "Status not IN_USE");

  // Original card, shall log out
  simulate_rfid_card(rfid, logic, card_uid);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "Machine is not free");
  // Original card away
  simulate_rfid_card(rfid, logic, std::nullopt);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "Machine is not free");

  // Now new card must succeed
  simulate_rfid_card(rfid, logic, card_uid2);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::LOGGED_IN, logic.getStatus(), "Status not LOGGED_IN");

  // Card away
  simulate_rfid_card(rfid, logic, std::nullopt);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::IN_USE, logic.getStatus(), "Status not IN_USE");

  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getActiveUser().card_uid == card_uid2, "User UID is not expected");
  TEST_ASSERT_TRUE_MESSAGE(!logic.getMachine().isFree(), "Machine is free");
}

void test_user_autologoff()
{
  machine_init(logic, rfid);

  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getAutologoffDelay() == conf::machine::DEFAULT_AUTO_LOGOFF_DELAY, "Autologoff delay not default");

  logic.setAutologoffDelay(2s);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getAutologoffDelay() == 2s, "Autologoff delay not 2s");

  simulate_rfid_card(rfid, logic, get_test_uid(0));
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::LOGGED_IN, logic.getStatus(), "Status not LOGGED_IN");

  // Card away
  simulate_rfid_card(rfid, logic, std::nullopt);
  delay(1000);
  TEST_ASSERT_FALSE_MESSAGE(logic.getMachine().isFree(), "Machine is free");
  TEST_ASSERT_FALSE_MESSAGE(logic.getMachine().isAutologoffExpired(), "Autologoff not expired");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::IN_USE, logic.getStatus(), "Status not IN_USE");

  // Now shall expire afer 2s
  simulate_rfid_card(rfid, logic, std::nullopt);
  delay(1000);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isAutologoffExpired(), "Autologoff expired");

  logic.logout();
  simulate_rfid_card(rfid, logic, std::nullopt);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "Machine is free");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::FREE, logic.getStatus(), "Status not FREE");
}

void test_machine_maintenance()
{
  machine_init(logic, rfid);

  auto &machine = logic.getMachineForTesting();
  const auto card_fabuser = get_test_uid(2);
  const auto card_admin = get_test_uid(0);

  machine.allowed = true;
  machine.maintenanceNeeded = true;
  simulate_rfid_card(rfid, logic, card_fabuser);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::MAINTENANCE_NEEDED, logic.getStatus(), "Status not MAINTENANCE_NEEDED");

  simulate_rfid_card(rfid, logic, std::nullopt);
  simulate_rfid_card(rfid, logic, card_admin, conf::machine::LONG_TAP_DURATION + 3s); // Log in + Conferma manutenzione perché non ritorna prima della conclusione
  simulate_rfid_card(rfid, logic, std::nullopt);                                      // Card away
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::IN_USE, logic.getStatus(), "Status not IN_USE by admin");
  TEST_ASSERT_FALSE_MESSAGE(logic.getMachine().maintenanceNeeded, "Maintenance not cleared by admin");

  // Logoff admin
  simulate_rfid_card(rfid, logic, card_admin);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "Machine is not free");
  simulate_rfid_card(rfid, logic, std::nullopt);

  // Now try to logon with fabuser (should succeed because maintenance is cleared)
  simulate_rfid_card(rfid, logic, card_fabuser);
  simulate_rfid_card(rfid, logic, std::nullopt);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::IN_USE, logic.getStatus(), "Status not IN_USE by normal user");
}

void test_machine_allowed()
{
  machine_init(logic, rfid);

  auto &machine = logic.getMachineForTesting();
  auto card_fabuser = get_test_uid(3);
  auto card_admin = get_test_uid(0);

  machine.allowed = false;
  machine.maintenanceNeeded = false;

  // check if blocked for normal users
  simulate_rfid_card(rfid, logic, card_fabuser);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::NOT_ALLOWED, logic.getStatus(), "Status not NOT_ALLOWED");
  simulate_rfid_card(rfid, logic, std::nullopt);

  // still blocked for admins
  simulate_rfid_card(rfid, logic, card_admin);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(BoardLogic::Status::NOT_ALLOWED, logic.getStatus(), "Status not NOT_ALLOWED for admins");
  simulate_rfid_card(rfid, logic, std::nullopt);
}

void tearDown(void){};

void setUp(void)
{
  TEST_ASSERT_TRUE_MESSAGE(SavedConfig::DefaultConfig().SaveToEEPROM(), "Default config save failed");
  TEST_ASSERT_TRUE_MESSAGE(logic.configure(rfid, lcd), "BoardLogic configure failed");
  TEST_ASSERT_TRUE_MESSAGE(logic.board_init(), "BoardLogic init failed");
  logic.setWhitelist(test_whitelist);
  // Disable MQTT for tests
  if (auto server_config = SavedConfig::LoadFromEEPROM(); server_config.has_value())
  {
    auto conf = server_config.value();
    strncpy(conf.mqtt_server, "INVALID_SERVER\0", sizeof(conf.mqtt_server));
    logic.getServer().configure(conf);
  }
};

void setup()
{
  delay(1000);
  UNITY_BEGIN();
  RUN_TEST(test_machine_defaults);
  RUN_TEST(test_simple_methods);
  RUN_TEST(test_machine_allowed);
  RUN_TEST(test_machine_maintenance);
  RUN_TEST(test_whitelist_no_network);
  RUN_TEST(test_one_user_at_a_time);
  RUN_TEST(test_user_autologoff);
  UNITY_END(); // stop unit testing
};

void loop()
{
}
