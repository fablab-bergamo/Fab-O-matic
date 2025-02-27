#ifndef CONF_H_
#define CONF_H_

#include <cstdint>
#include <string>
#include <chrono>

#include "MachineID.hpp"

namespace fabomatic
{
  using namespace std::chrono_literals;

  /// @brief Machine-related default settings
  namespace conf::default_config
  {
    /// @brief Default MachineID for backend. Can be overriden through WiFi Portal config
    inline constexpr MachineID machine_id{1};

    /// @brief Default machine name for LCD. Will be overriden with Backend config data
    inline constexpr std::string_view machine_name{"MACHINE1"};

    /// @brief Default machine type. No impact on code
    inline constexpr MachineType machine_type{MachineType::Printer3D};

    /// @brief Default hostname for the ESP32 board, Machine ID will be added to the hostname in order to form unique hostnames
    inline constexpr std::string_view hostname{"BOARD"};

  } // namespace conf::default_config

  /// @brief RFID-related settings
  namespace conf::rfid_tags
  {
    /// @brief Number of bytes in RFID cards UID, may depend on specific RFID chip
    inline constexpr uint8_t UID_BYTE_LEN{4};
    /// @brief Number of cached UID, persisted in flash
    inline constexpr uint8_t CACHE_LEN{10};

  } // namespace conf::rfid_tags

  /// @brief Configuration for LCD pannel
  namespace conf::lcd
  {
    /// @brief Number of rows for LCD display
    inline constexpr uint8_t ROWS{2};

    /// @brief Number of cols for LCD display
    inline constexpr uint8_t COLS{16};

    /// @brief How much time shall we wait for a short message on LCD for user
    inline constexpr auto SHORT_MESSAGE_DELAY{1s};

  } // namespace conf::lcd

  /// @brief Configuration for connected machine
  namespace conf::machine
  {
    /// @brief User will be log out after this delay. If 0h, no auto-logout. This may be overriden by backend data
    inline constexpr auto DEFAULT_AUTO_LOGOFF_DELAY{12h};

    /// @brief Idle time before poweroff. If 0min, machine will stay on.
    inline constexpr auto DEFAULT_GRACE_PERIOD{90s};

    /// @brief Beeps will be heard every 30s when the machine is in grace period
    inline constexpr auto DELAY_BETWEEN_BEEPS{30s};

    /// @brief If true, machine needing maintenance will be blocked for normal users
    inline constexpr bool MAINTENANCE_BLOCK{true};

    /// @brief Minimum time to confirm by long tap maintenance
    inline constexpr auto LONG_TAP_DURATION{10s};

    /// @brief Disabled RFID reading after a successfull read for X seconds.
    inline constexpr auto DELAY_BETWEEN_SWEEPS{2s};

  } // namespace conf::machine

  /// @brief Debug settings
  namespace conf::debug
  {
    /// @brief True to add logs to serial output
    inline constexpr bool ENABLE_LOGS{true};

    /// @brief True to add many logs regarding tasks scheduling and statistics
    inline constexpr bool ENABLE_TASK_LOGS{false};

    /// @brief Serial speed in bauds
    inline constexpr unsigned long SERIAL_SPEED_BDS{115200};

    /// @brief True to force portal startup. May be useful to override saved configuration
    inline constexpr bool FORCE_PORTAL{false};

    /// @brief True to force EEPROM settings to defaults, regardless of actual values.
    inline constexpr bool LOAD_EEPROM_DEFAULTS{false};

    /// @brief True if important MQTT messages should be saved when network is down and replayed.
    inline constexpr bool ENABLE_BUFFERING{true};

  } // namespace conf::debug

  /// @brief Configuration for buzzer
  namespace conf::buzzer
  {
    /// @brief Single beep duration, typical value 200ms. Set to 0 to disable beeps.
    inline constexpr auto STANDARD_BEEP_DURATION{250ms};

    /// @brief Number of beeps every time the function is called
    inline constexpr auto NB_BEEPS{3};

  } // namespace conf::buzzer

  /// @brief Configuration related to tasks scheduling
  namespace conf::tasks
  {
    /// @brief Task period to check for RFID badge (should be fast: 150ms)
    inline constexpr auto RFID_CHECK_PERIOD{150ms};

    /// @brief Performs RFID self check and reset chip if necessary (default: 60s)
    inline constexpr auto RFID_SELFTEST_PERIOD{60s};

    /// @brief Query the MQTT broker for machine state at given period (default: 30s)
    inline constexpr auto MQTT_REFRESH_PERIOD{30s};

    /// @brief Timeout for hardware watchdog, set to 0s to disable (default: 60s)
    inline constexpr auto WATCHDOG_TIMEOUT{60s};

    /// @brief Period of the watchdog signaling task (default 1s)
    inline constexpr auto WATCHDOG_PERIOD{1s};

    /// @brief Timeout for portal configuration (default: 5min)
    inline constexpr auto PORTAL_CONFIG_TIMEOUT{5min};

    /// @brief Board announcement on the MQTT server (default: 2min)
    inline constexpr auto MQTT_ALIVE_PERIOD{2min};

  } // namespace conf::tasks

  /// @brief Configuration regarding MQTT broker, topics
  namespace conf::mqtt
  {
    /// @brief Initial part of the topic, machine ID will be added
    inline constexpr std::string_view topic{"machine"};

    /// @brief Backend reply (sub-topic of the full machine topic)
    inline constexpr std::string_view response_topic{"/reply"};

    /// @brief Backend requests (sub-topic of the full machine topic)
    inline constexpr std::string_view request_topic{"/request"};

    /// @brief Number of tries to get a reply from the backend
    inline constexpr auto MAX_TRIES{1};

    /// @brief Timeout for a single backend reply request.
    inline constexpr auto TIMEOUT_REPLY_SERVER{2s};

    /// @brief Once backend is unresponsive, wait at least this period before to try again
    inline constexpr auto FAIL_FAST_PERIOD {45s};

    /// @brief MQTT port for broker
    inline constexpr auto PORT_NUMBER{1883};

    /// @brief Name of the default server for Backend. Will be resolved through mDNS
    inline constexpr std::string_view mqtt_server{"fabpi2.local"};

    /// @brief In case Shelly is used, name of the topic on MQTT Broker
    inline constexpr std::string_view mqtt_switch_topic{""};

    /// @brief What value shall be written on the topic to switch on the Shelly device
    inline constexpr std::string_view mqtt_switch_on_message{"on"};

    /// @brief What value shall be written on the topic to switch off the Shelly device
    inline constexpr std::string_view mqtt_switch_off_message{"off"};
  } // namespace conf::mqtt

  /// @brief Other compile-time settings
  namespace conf::common
  {
    /// @brief Maximum length of saved string in WiFiManager portal.
    inline constexpr auto STR_MAX_LENGTH{40U};

    /// @brief Maximum length of saved integer in WiFiManager portal.
    inline constexpr auto INT_MAX_LENGTH{5U};
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