#ifndef CONF_H_
#define CONF_H_

#include <cstdint>
#include <string>

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
  constexpr uint16_t TIMEOUT_USAGE_MINUTES = 8 * 60;         /* User will be log out after this delay. If 0, no auto-logout. */
  constexpr unsigned long BEEP_REMAINING_MS = 2 * 60 * 1000; /* Device will beep before auto-poweroff. If 0, no beeping.  */
  constexpr unsigned long POWEROFF_DELAY_MS = 5 * 60 * 1000; /* Milliseconds of idle time before poweroff. If 0, machine will stay on. */
  constexpr uint16_t DELAY_BETWEEN_BEEPS_S = 45;             /* When about to shutdown, it will beep every X seconds */
  constexpr bool MAINTENANCE_BLOCK = true;                   /* If true, machine needing maintenance will be blocked for normal users */
  static_assert(BEEP_REMAINING_MS <= POWEROFF_DELAY_MS);
}

namespace conf::debug
{
  constexpr bool ENABLE_LOGS = true;                 /* True to add logs */
  constexpr bool ENABLE_TASK_LOGS = false;           /* True to add logs */
  constexpr unsigned long SERIAL_SPEED_BDS = 115200; /* Serial speed in bauds */
  constexpr bool FAKE_BACKEND = true;                /* True to use fake broker */
}
namespace conf::buzzer
{
  constexpr unsigned short LEDC_PWM_CHANNEL = 0U;    /* Esp32 pwm channel for beep generation */
  constexpr unsigned short BEEP_DURATION_MS = 200UL; /* Beep duration in milliseconds */
  constexpr unsigned int BEEP_HZ = 660U;
}
namespace conf::tasks
{
  constexpr unsigned int RFID_CHECK_MS = 150;     /* Task period to check for RFID badge in milliseconds */
  constexpr unsigned int RFID_CHIP_CHECK_S = 60;  /* Performs RFID self check and reset if necessary every X seconds */
  constexpr uint16_t REFRESH_PERIOD_SECONDS = 30; /* Notify the server every X seconds */
  constexpr int WDG_TIMEOUT_S = 30;               /* Timeout for hardware watchdog in seconds, set to 0 to disable */
  static_assert(WDG_TIMEOUT_S == 0 || WDG_TIMEOUT_S > 5);
}

#endif // CONF_H_