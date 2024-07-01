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
    /**
     * Name of the default server for Backend. Will be resolved through mDNS
     */
    static constexpr std::string_view mqtt_server{"fabpi2.local"};

    /**
     * In case Shelly is used, name of the topic on MQTT Broker
     */
    static constexpr std::string_view mqtt_switch_topic{""};

    /**
     * What value shall be written on the topic to switch on the Shelly device
     */
    static constexpr std::string_view mqtt_switch_on_message{"on"};

    /**
     * What value shall be written on the topic to switch off the Shelly device
     */
    static constexpr std::string_view mqtt_switch_off_message{"off"};

    /**
     * Default MachineID for backend. Can be overriden through WiFi Portal config
     */
    static constexpr MachineID machine_id{1};

    /**
     * Default machine name for LCD. Will be overriden with Backend config data
     */
    static constexpr std::string_view machine_name{"MACHINE1"};

    /**
     * Default machine type. No impact on code
     */
    static constexpr MachineType machine_type{MachineType::Printer3D};

    /**
     * Default hostname for the ESP32 board, Machine ID will be added to the hostname in order to form unique hostnames
     */
    static constexpr std::string_view hostname{"BOARD"};

  } // namespace conf::default_config

  /**
   * Configuration for RFID related settings
   */
  namespace conf::rfid_tags
  {
    /* Number of bytes in RFID cards UID, may depend on specific RFID chip */
    static constexpr uint8_t UID_BYTE_LEN{4};
    /* Number of cached UID, persisted in flash  */
    static constexpr uint8_t CACHE_LEN{10};

  } // namespace conf::rfid_tags

  /**
   * Configuration for LCD pannel
   */
  namespace conf::lcd
  {
    /**
     * Number of rows for LCD display
     */
    static constexpr uint8_t ROWS{2};
    /**
     * Number of cols for LCD display
     */
    static constexpr uint8_t COLS{16};
    /**
     * How much time shall we wait for a short message on LCD for user
     */
    static constexpr auto SHORT_MESSAGE_DELAY{1s};

  } // namespace conf::lcd

  /**
   * Configuration for connected machine
   */
  namespace conf::machine
  {
    /**
     * User will be log out after this delay. If 0h, no auto-logout. This may be overriden by backend data
     */
    static constexpr auto DEFAULT_AUTO_LOGOFF_DELAY{12h};

    /**
     * Idle time before poweroff. If 0min, machine will stay on.
     */
    static constexpr auto DEFAULT_GRACE_PERIOD{90s};

    /**
     * Beeps will be heard every 30s when the machine is in grace period
     */
    static constexpr auto DELAY_BETWEEN_BEEPS{30s};

    /**
     * If true, machine needing maintenance will be blocked for normal users
     */
    static constexpr bool MAINTENANCE_BLOCK{true};

    /**
     * Minimum time to confirm by long tap maintenance
     */
    static constexpr auto LONG_TAP_DURATION{10s};

  } // namespace conf::machine

  namespace conf::debug
  {
    /**
     * True to add logs to serial output
     */
    static constexpr bool ENABLE_LOGS{true};
    /**
     * True to add many logs regarding tasks scheduling and statistics
     */
    static constexpr bool ENABLE_TASK_LOGS{false};
    /**
     * Serial speed in bauds
     */
    static constexpr unsigned long SERIAL_SPEED_BDS{115200};
    /**
     * True to force portal startup. May be useful to override saved configuration
     */
    static constexpr bool FORCE_PORTAL{false};
    /**
     * True to force EEPROM settings to defaults, regardless of actual values.
     */
    static constexpr bool LOAD_EEPROM_DEFAULTS{false};
    /**
     * True if important MQTT messages should be saved when network is down and replayed.
     */
    static constexpr bool ENABLE_BUFFERING{true};

  } // namespace conf::debug

  /**
   * Configuration for buzzer
   */
  namespace conf::buzzer
  {
    /**
     * Single beep duration, typical value 200ms. Set to 0 to disable beeps.
     */
    static constexpr auto STANDARD_BEEP_DURATION{250ms};
    /**
     * Number of beeps every time the function is called
     */
    static constexpr auto NB_BEEPS{3};

  } // namespace conf::buzzer

  /**
   * Configuration related to tasks scheduling
   */
  namespace conf::tasks
  {
    /**
     * Task period to check for RFID badge (should be fast: 150ms)
     */
    static constexpr auto RFID_CHECK_PERIOD{150ms};

    /**
     * Performs RFID self check and reset chip if necessary (default: 60s)
     */
    static constexpr auto RFID_SELFTEST_PERIOD{60s};

    /**
     * Query the MQTT broker for machine state at given period (default: 30s)
     */
    static constexpr auto MQTT_REFRESH_PERIOD{30s};

    /**
     * Timeout for hardware watchdog, set to 0s to disable (default: 60s)
     */
    static constexpr auto WATCHDOG_TIMEOUT{60s};

    /**
     * Period of the watchdog signaling task (default 1s)
     */
    static constexpr auto WATCHDOG_PERIOD{1s};

    /**
     * Timeout for portal configuration (default: 5min)
     */
    static constexpr auto PORTAL_CONFIG_TIMEOUT{5min};

    /**
     * Board announcement on the MQTT server (default: 2min)
     */
    static constexpr auto MQTT_ALIVE_PERIOD{2min};

  } // namespace conf::tasks

  /**
   * Configuration regarding MQTT broker, topics
   */
  namespace conf::mqtt
  {
    /**
     * Initial part of the topic, machine ID will be added
     */
    static constexpr std::string_view topic{"machine"};

    /**
     * Backend reply (sub-topic of the full machine topic)
     */
    static constexpr std::string_view response_topic{"/reply"};

    /**
     * Number of tries to get a reply from the backend
     */
    static constexpr auto MAX_TRIES{2};

    /**
     * Timeout for a single backend reply request.
     */
    static constexpr auto TIMEOUT_REPLY_SERVER{2s};

    /**
     * MQTT port for broker
     */
    static constexpr auto PORT_NUMBER{1883};
  } // namespace conf::mqtt

  namespace conf::common
  {
    /**
     * Maximum length of saved string in WiFiManager portal.
     */
    static constexpr auto STR_MAX_LENGTH{40U};
    /**
     * Maximum length of saved integer in WiFiManager portal.
     */
    static constexpr auto INT_MAX_LENGTH{5U};
  }

  // Checks on configured values
  static_assert(conf::mqtt::topic.size() < conf::common::STR_MAX_LENGTH, "MQTT topic too long");
  static_assert(conf::mqtt::response_topic.size() < conf::common::STR_MAX_LENGTH, "MQTT response too long");
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