#include <chrono>
#include <string>
#include <functional>
#include <vector>

#include <Arduino.h>
#define UNITY_INCLUDE_PRINT_FORMATTED
#include <unity.h>
#include "FabServer.hpp"
#include "mock/MockLCDWrapper.hpp"
#include "mock/MockRFIDWrapper.hpp"
#define XTRA_UNIT_TEST
#include "BoardLogic.hpp"
#include "conf.hpp"
#include "SavedConfig.hpp"

using namespace fablabbg;
using namespace std::chrono;
using namespace std::chrono_literals;

FabServer server;
MockRFIDWrapper rfid;
MockLCDWrapper lcd;
BoardLogic logic;

static constexpr WhiteList test_whitelist /* List of RFID tags whitelisted, regardless of connection */
    {
        std::make_tuple(0xAABBCCD1, FabUser::UserLevel::FABLAB_ADMIN, "ABCDEFG"),
        std::make_tuple(0xAABBCCD2, FabUser::UserLevel::FABLAB_STAFF, "PIPPO"),
        std::make_tuple(0xAABBCCD3, FabUser::UserLevel::FABLAB_USER, "USER1"),
        std::make_tuple(0xAABBCCD4, FabUser::UserLevel::FABLAB_USER, "USER2"),
        std::make_tuple(0xAABBCCD5, FabUser::UserLevel::FABLAB_USER, "USER3")};

void tearDown(void)
{
}

void setUp(void)
{
  TEST_ASSERT_TRUE_MESSAGE(SavedConfig::DefaultConfig().SaveToEEPROM(), "Default config save failed");
  TEST_ASSERT_TRUE_MESSAGE(logic.configure(server, rfid, lcd), "BoardLogic configure failed");
  TEST_ASSERT_TRUE_MESSAGE(logic.board_init(), "BoardLogic init failed");
  TEST_ASSERT_FALSE_MESSAGE(lcd.last_boardinfo.server_connected, "Server not connected");
  logic.setWhitelist(test_whitelist);
}

card::uid_t get_test_uid(size_t idx = 0)
{
  auto [card_uid, level, name] = test_whitelist[idx];
  return card_uid;
}

void machine_init()
{
  auto &machine = logic.getMachine();
  machine.allowed = true;
  machine.maintenanceNeeded = false;
  logic.logout();
  rfid.resetUid();
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::FREE, "machine_init: Status not FREE");
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "machine_init: machine not free");
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
    auto result = logic.getStatus() == status;
    TEST_ASSERT_TRUE_MESSAGE(result, "Status not PORTAL_OK");
  }

  logic.checkPowerOff();
  logic.invert_led();
  logic.led(true);
  logic.led(false);
  logic.refreshLCD();
  logic.set_led_color(random(0, 255), random(0, 255), random(0, 255));
  logic.led(true);
  logic.updateLCD();
  logic.refreshLCD();
}

void test_whitelist_no_network()
{
  machine_init();

  // Check whitelist is recognized
  for (auto &wle : test_whitelist)
  {
    auto [card_uid, level, name] = wle;
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
  auto [card_uid, level, name] = test_whitelist[1];

  rfid.setUid(card_uid);
  TEST_ASSERT_TRUE_MESSAGE(card_uid == rfid.getUid(), "Card UID not equal");
  TEST_ASSERT_TRUE_MESSAGE(rfid.isNewCardPresent(), "New card not present");
  TEST_ASSERT_TRUE_MESSAGE(rfid.readCardSerial().has_value(), "Card serial not read");
  TEST_ASSERT_TRUE_MESSAGE(rfid.readCardSerial().value() == card_uid, "Card serial not equal");

  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::LOGGED_IN, "Status not LOGGED_IN");
  logic.logout();
  rfid.resetUid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::LOGOUT, "Status not LOGOUT");
}

void test_one_user_at_a_time()
{
  // Check that machine is free
  machine_init();

  auto [card_uid, level, name] = test_whitelist[0];
  rfid.setUid(card_uid);
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::LOGGED_IN, "Status not LOGGED_IN");
  TEST_ASSERT_TRUE_MESSAGE(!logic.getMachine().isFree(), "Machine is free");

  auto [card_uid2, level2, name2] = test_whitelist[1];
  rfid.setUid(card_uid2);
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getActiveUser().card_uid == card_uid, "User UID has changed");

  logic.checkRfid();
  rfid.resetUid();
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::IN_USE, "Status not IN_USE");
  logic.logout();

  // Now must succeed
  rfid.setUid(card_uid2);
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::LOGGED_IN, "Status not LOGGED_IN");
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getActiveUser().card_uid == card_uid2, "User UID has changed");
}

void test_user_autologoff()
{
  machine_init();

  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getAutologoffDelay() == conf::machine::DEFAULT_AUTO_LOGOFF_DELAY, "Autologoff delay not default");

  logic.setAutologoffDelay(2s);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().getAutologoffDelay() == 2s, "Autologoff delay not 2s");

  rfid.setUid(get_test_uid(0));
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::LOGGED_IN, "Status not LOGGED_IN");
  delay(1000);
  logic.checkRfid();
  TEST_ASSERT_FALSE_MESSAGE(logic.getMachine().isFree(), "Machine is free");
  TEST_ASSERT_FALSE_MESSAGE(logic.getMachine().isAutologoffExpired(), "Autologoff not expired");
  delay(1000);
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isAutologoffExpired(), "Autologoff expired");
  logic.logout();
  TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "Machine is free");
  rfid.resetUid();
}

void test_machine_maintenance()
{
  // check maintenance
  machine_init();
  auto &machine = logic.getMachine();

  machine.allowed = true;
  machine.maintenanceNeeded = true;
  rfid.setUid(get_test_uid(2)); // USER
  logic.checkRfid();
  rfid.resetUid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::MAINTENANCE_NEEDED, "Status not MAINTENANCE_NEEDED");

  logic.checkRfid();
  rfid.setUid(get_test_uid(0)); // ADMIN can mark maintenance done
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() != BoardLogic::Status::MAINTENANCE_NEEDED, "Status MAINTENANCE_NEEDED for admin");
}

void test_machine_allowed()
{
  machine_init();
  auto &machine = logic.getMachine();

  machine.allowed = false;
  machine.maintenanceNeeded = false;

  // check if blocked for normal users
  rfid.setUid(get_test_uid(3)); // USER
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::NOT_ALLOWED, "Status not NOT_ALLOWED");

  // still blocked for admins
  rfid.setUid(get_test_uid(0)); // ADMIN
  logic.checkRfid();
  TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::NOT_ALLOWED, "Status not NOT_ALLOWED for admins");
}

void setup()
{
  delay(2000); // service delay
  UNITY_BEGIN();
  RUN_TEST(test_machine_defaults);
  RUN_TEST(test_simple_methods);
  RUN_TEST(test_machine_allowed);
  RUN_TEST(test_machine_maintenance);
  RUN_TEST(test_whitelist_no_network);
  RUN_TEST(test_one_user_at_a_time);
  RUN_TEST(test_user_autologoff);
  UNITY_END(); // stop unit testing
}

void loop()
{
}