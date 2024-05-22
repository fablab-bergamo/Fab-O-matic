#ifndef CONF_H_
#define CONF_H_

#include <cstdint>
#include <string>
#include <chrono>

#include "MachineID.hpp"

namespace fabomatic
{
  using namespace std::chrono_literals;

  namespace conf::default_config
  {
    static constexpr std::string_view mqtt_server = "fabpi2.local";
    static constexpr std::string_view mqtt_switch_topic = "";
    static constexpr std::string_view mqtt_switch_on_message{"on"};
    static constexpr std::string_view mqtt_switch_off_message{"off"};
    static constexpr MachineID machine_id{1};
    static constexpr std::string_view machine_name = "MACHINE1";
    static constexpr MachineType machine_type = MachineType::Printer3D;
    static constexpr std::string_view hostname = "BOARD"; // Machine ID will be added to the hostname in order to form unique hostnames

  } // namespace conf::default_config

  namespace conf::rfid_tags
  {
    static constexpr uint8_t UID_BYTE_LEN{4}; /* Number of bytes in RFID cards UID */
    static constexpr uint8_t CACHE_LEN{10};   /* Number of cached UID */

  } // namespace conf::rfid_tags

  namespace conf::lcd
  {
    static constexpr uint8_t ROWS{2};              /* Number of rows for LCD display */
    static constexpr uint8_t COLS{16};             /* Number of cols for LCD display */
    static constexpr auto SHORT_MESSAGE_DELAY{1s}; /* How much time shall we wait for a short message on LCD for user */

  } // namespace conf::lcd

  namespace conf::machine
  {
    static constexpr auto DEFAULT_AUTO_LOGOFF_DELAY{12h}; /* User will be log out after this delay. If 0h, no auto-logout. This may be overriden by backend data */
    static constexpr auto BEEP_PERIOD{2min};              /* Device will beep before auto-poweroff. If 0min, no beeping.  */
    static constexpr auto DEFAULT_GRACE_PERIOD{5min};     /* Idle time before poweroff. If 0min, machine will stay on. */
    static constexpr auto DELAY_BETWEEN_BEEPS{30s};       /* Beeps will be heard every 30s when the machine is about to shutdown */
    static constexpr bool MAINTENANCE_BLOCK{true};        /* If true, machine needing maintenance will be blocked for normal users */
    static constexpr auto LONG_TAP_DURATION{10s};         /* Minimum time to confirm by long tap maintenance*/

  } // namespace conf::machine

  namespace conf::debug
  {
    static constexpr bool ENABLE_LOGS{true};                 /* True to add logs */
    static constexpr bool ENABLE_TASK_LOGS{false};           /* True to add logs regarding tasks scheduling and statistics */
    static constexpr unsigned long SERIAL_SPEED_BDS{115200}; /* Serial speed in bauds */
    static constexpr bool FORCE_PORTAL{false};               /* True to force portal startup */
    static constexpr bool LOAD_EEPROM_DEFAULTS{false};       /* True to force EEPROM settings to defaults */

  } // namespace conf::debug

  namespace conf::buzzer
  {
    static constexpr auto STANDARD_BEEP_DURATION{250ms}; /* Single beep duration, typical value 200ms. Set to 0 to disable beeps. */
    static constexpr auto NB_BEEPS{3};                   /* Number of beeps every time the function is callsed */

  } // namespace conf::buzzer

  namespace conf::tasks
  {
    static constexpr auto RFID_CHECK_PERIOD{150ms};    /* Task period to check for RFID badge (should be fast: 150ms) */
    static constexpr auto RFID_SELFTEST_PERIOD{60s};   /* Performs RFID self check and reset chip if necessary (default: 60s) */
    static constexpr auto MQTT_REFRESH_PERIOD{30s};    /* Query the MQTT broker for machine state at given period (default: 30s) */
    static constexpr auto WATCHDOG_TIMEOUT{60s};       /* Timeout for hardware watchdog, set to 0s to disable (default: 60s) */
    static constexpr auto WATCHDOG_PERIOD{1s};         /* Period of the watchdog signaling task*/
    static constexpr auto PORTAL_CONFIG_TIMEOUT{5min}; /* Timeout for portal configuration (default: 5min) */
    static constexpr auto MQTT_ALIVE_PERIOD{2min};     /* Board announcement on the MQTT server (default: 2min)  */

  } // namespace conf::tasks

  namespace conf::mqtt
  {
    static constexpr std::string_view topic{"machine"};         /* Initial part of the topic, machine ID will be added */
    static constexpr std::string_view response_topic{"/reply"}; /* Backend reply (sub-topic of the full machine topic) */
    static constexpr auto MAX_TRIES{2};                         /* Number of tries to get a reply from the backend */
    static constexpr auto TIMEOUT_REPLY_SERVER{2s};             /* Timeout for a single backend reply request. */
    static constexpr auto PORT_NUMBER{1883};                    /* MQTT port for broker */
  } // namespace conf::mqtt

  namespace conf::common
  {
    static constexpr auto STR_MAX_LENGTH{40U}; /* Maximum length of saved string in WiFiManager portal. */
    static constexpr auto INT_MAX_LENGTH{5U};  /* Maximum length of saved integer in WiFiManager portal. */
  }

  // Checks on configured values
  static_assert(conf::mqtt::topic.size() < conf::common::STR_MAX_LENGTH, "MQTT topic too long");
  static_assert(conf::mqtt::response_topic.size() < conf::common::STR_MAX_LENGTH, "MQTT response too long");
  static_assert(conf::machine::BEEP_PERIOD <= conf::machine::DEFAULT_GRACE_PERIOD, "BEEP_PERIOD must be <= POWEROFF_GRACE_PERIOD");
  static_assert(conf::machine::DELAY_BETWEEN_BEEPS < conf::machine::BEEP_PERIOD, "DELAY_BETWEEN_BEEPS must be < BEEP_PERIOD");
  static_assert(conf::buzzer::STANDARD_BEEP_DURATION <= 1s, "STANDARD_BEEP_DURATION must be <= 1s");
  static_assert(conf::mqtt::TIMEOUT_REPLY_SERVER > 500ms, "TIMEOUT_REPLY_SERVER must be > 500ms");
  static_assert(conf::mqtt::MAX_TRIES > 0, "MAX_TRIES must be > 0");

  // Make sure the hardware watchdog period is not too short considering we're blocking tasks for some operations
  static_assert(conf::tasks::WATCHDOG_TIMEOUT == 0s ||
                    conf::tasks::WATCHDOG_TIMEOUT > (conf::machine::LONG_TAP_DURATION +
                                                     conf::mqtt::TIMEOUT_REPLY_SERVER * conf::mqtt::MAX_TRIES * 3 +
                                                     conf::lcd::SHORT_MESSAGE_DELAY * 3 +
                                                     conf::buzzer::STANDARD_BEEP_DURATION * conf::buzzer::NB_BEEPS * 2 +
                                                     5s),
                "Watchdog period too short");
  static_assert(conf::tasks::WATCHDOG_PERIOD > 0s && conf::tasks::WATCHDOG_PERIOD * 10 < conf::tasks::WATCHDOG_TIMEOUT, "WATCHDOG_PERIOD must be small relative to WATCHDOG_TIMEOUT");
} // namespace fabomatic
#endif // CONF_H_