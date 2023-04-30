#ifndef CONF_H_
#define CONF_H_

#include <cstdint>
#include <string>
#include <chrono>

using namespace std::chrono_literals;
using namespace std::chrono;

namespace conf::whitelist
{
  constexpr uint8_t LEN = 10U;         /* Maximum number of whitelisted cards */
  constexpr uint8_t UID_BYTE_LEN = 4U; /* Number of bytes in RFID cards UID */
  constexpr uint8_t CACHE_LEN = 10U;   /* Number of bytes in RFID cards UID */
}
namespace conf::lcd
{
  constexpr uint8_t ROWS = 2U;  /* Number of rows for LCD display */
  constexpr uint8_t COLS = 16U; /* Number of cols for LCD display */
}
namespace conf::machine
{
  constexpr auto AUTO_LOGOFF_DELAY = 8h;       /* User will be log out after this delay. If 0h, no auto-logout. */
  constexpr auto BEEP_PERIOD = 1min;           /* Device will beep before auto-poweroff. If 0min, no beeping.  */
  constexpr auto POWEROFF_GRACE_PERIOD = 1min; /* Idle time before poweroff. If 0min, machine will stay on. */
  constexpr auto DELAY_BETWEEN_BEEPS = 30s;    /* Beeps will be heard every 30s when the machine is about to shutdown */
  constexpr bool MAINTENANCE_BLOCK = true;     /* If true, machine needing maintenance will be blocked for normal users */

  static_assert(BEEP_PERIOD <= POWEROFF_GRACE_PERIOD);
  static_assert(DELAY_BETWEEN_BEEPS < BEEP_PERIOD);
}

namespace conf::debug
{
  constexpr bool ENABLE_LOGS = true;                 /* True to add logs */
  constexpr bool ENABLE_TASK_LOGS = false;           /* True to add logs */
  constexpr unsigned long SERIAL_SPEED_BDS = 115200; /* Serial speed in bauds */
}
namespace conf::buzzer
{
  constexpr unsigned short LEDC_PWM_CHANNEL = 0U; /* Esp32 pwm channel for beep generation */
  constexpr auto STANDARD_BEEP_DURATION = 200ms;  /* Beep duration, typical value 200ms */
  constexpr unsigned int BEEP_HZ = 660U;
}
namespace conf::tasks
{
  constexpr auto RFID_CHECK_PERIOD = 150ms;  /* Task period to check for RFID badge (should be fast: 150ms) */
  constexpr auto RFID_SELFTEST_PERIOD = 60s; /* Performs RFID self check and reset chip if necessary (default: 60s) */
  constexpr auto MQTT_REFRESH_PERIOD = 30s;  /* Query the MQTT broker for machine state at given period (default: 30s) */
  constexpr auto WATCHDOG_TIMEOUT = 30s;     /* Timeout for hardware watchdog set to 0s to disable (default: 30s) */
  static_assert(WATCHDOG_TIMEOUT == 0s || WATCHDOG_TIMEOUT > 10s);
}

#endif // CONF_H_