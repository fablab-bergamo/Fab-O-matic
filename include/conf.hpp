#ifndef CONF_H_
#define CONF_H_

#include <cstdint>
#include <string>
#include <chrono>

#include "MachineConfig.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

namespace fablabbg
{
  namespace conf::default_config
  {
    static constexpr std::string_view mqtt_server = "127.0.0.1";
    static constexpr std::string_view machine_topic = "shelly/command/switch:0";
    static constexpr MachineID machine_id{1};
    static constexpr std::string_view machine_name = "MACHINE1";
    static constexpr MachineType machine_type = MachineType::LASER;
  }
  namespace conf::rfid_tags
  {
    static constexpr uint8_t UID_BYTE_LEN{4}; /* Number of bytes in RFID cards UID */
    static constexpr uint8_t CACHE_LEN{10};   /* Number of cached UID */
  }
  namespace conf::lcd
  {
    static constexpr uint8_t ROWS{2};              /* Number of rows for LCD display */
    static constexpr uint8_t COLS{16};             /* Number of cols for LCD display */
    static constexpr auto SHORT_MESSAGE_DELAY{1s}; /* How much time shall we wait for a short message on LCD for user */
  }
  namespace conf::machine
  {
    static constexpr auto DEFAULT_AUTO_LOGOFF_DELAY{8h}; /* User will be log out after this delay. If 0h, no auto-logout. This may be overriden by backend data */
    static constexpr auto BEEP_PERIOD{1min};             /* Device will beep before auto-poweroff. If 0min, no beeping.  */
    static constexpr auto POWEROFF_GRACE_PERIOD{2min};   /* Idle time before poweroff. If 0min, machine will stay on. */
    static constexpr auto DELAY_BETWEEN_BEEPS{30s};      /* Beeps will be heard every 30s when the machine is about to shutdown */
    static constexpr bool MAINTENANCE_BLOCK{true};       /* If true, machine needing maintenance will be blocked for normal users */
    static constexpr auto LONG_TAP_DURATION{10s};        /* Minimum time to confirm by long tap maintenance*/
    static_assert(BEEP_PERIOD <= POWEROFF_GRACE_PERIOD);
    static_assert(DELAY_BETWEEN_BEEPS < BEEP_PERIOD);
  }

  namespace conf::debug
  {
    static constexpr bool ENABLE_LOGS{true};                 /* True to add logs */
    static constexpr bool ENABLE_TASK_LOGS{false};           /* True to add logs regarding tasks scheduling and statistics */
    static constexpr unsigned long SERIAL_SPEED_BDS{115200}; /* Serial speed in bauds */
    static constexpr bool FORCE_PORTAL_RESET{false};         /* True to force EEPROM reset */
  }
  namespace conf::buzzer
  {
    static constexpr unsigned short LEDC_PWM_CHANNEL{0}; /* Esp32 pwm channel for beep generation */
    static constexpr auto STANDARD_BEEP_DURATION{200ms}; /* Single beep duration, typical value 200ms */
    static constexpr auto NB_BEEPS{3};                   /* Number of beeps every time the function is callsed */
    static constexpr unsigned int BEEP_HZ{660};          /* Beep frequency in Hz */
  }
  namespace conf::tasks
  {
    static constexpr auto RFID_CHECK_PERIOD{150ms};    /* Task period to check for RFID badge (should be fast: 150ms) */
    static constexpr auto RFID_SELFTEST_PERIOD{60s};   /* Performs RFID self check and reset chip if necessary (default: 60s) */
    static constexpr auto MQTT_REFRESH_PERIOD{30s};    /* Query the MQTT broker for machine state at given period (default: 30s) */
    static constexpr auto WATCHDOG_TIMEOUT{30s};       /* Timeout for hardware watchdog, set to 0s to disable (default: 30s) */
    static constexpr auto PORTAL_CONFIG_TIMEOUT{5min}; /* Timeout for portal configuration (default: 5min) */
  }

  namespace conf::mqtt
  {
    static constexpr std::string_view topic{"machine"};         /* Initial part of the topic, machine ID will be added */
    static constexpr std::string_view response_topic{"/reply"}; /* Server reply (sub-topic of the full machine topic) */
    static constexpr auto MAX_RETRY_DURATION{2s};               /* How much time should we spend at maximum trying to contact the broker for a single request */
  }

  // Make sure the hardware watchdog period is not too short considering we're blocking tasks for some operations
  static_assert(conf::tasks::WATCHDOG_TIMEOUT == 0s ||
                    conf::tasks::WATCHDOG_TIMEOUT > (conf::machine::LONG_TAP_DURATION +
                                                     conf::mqtt::MAX_RETRY_DURATION * 3 +
                                                     conf::lcd::SHORT_MESSAGE_DELAY * 3 +
                                                     conf::buzzer::STANDARD_BEEP_DURATION * conf::buzzer::NB_BEEPS * 2 +
                                                     1s),
                "Watchdog period too short");
} // namespace fablabbg
#endif // CONF_H_