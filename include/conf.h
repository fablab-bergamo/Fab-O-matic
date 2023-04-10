#ifndef _CONF_H_
#define _CONF_H_

#include <cstdint>
#include <string>

namespace conf
{
    namespace whitelist
    {
        constexpr uint8_t LEN = 10U; /* Maximum number of whitelisted cards */
        constexpr uint8_t UID_BYTE_LEN = 8U; /* Number of bytes in RFID cards UID */
        constexpr uint8_t CACHE_LEN = 10U; /* Number of bytes in RFID cards UID */
    }
    namespace lcd
    {
        constexpr uint8_t ROWS = 2U;  /* Number of rows for LCD */
        constexpr uint8_t COLS = 16U; /* Number of cols for LCD */
    }
    namespace machine
    {
        constexpr uint16_t TIMEOUT_USAGE_MINUTES = 8 * 60; /* User will be log out after this delay */
        constexpr uint16_t BEEP_REMAINING_MINUTES = 1; /* Device will beep before auto-poweroff. If 0, no beeping.  */
        constexpr uint16_t POWEROFF_DELAY_MINUTES = 2; /* Minutes of idle time before poweroff. If 0, machine will stay on. */
        static_assert(BEEP_REMAINING_MINUTES <= POWEROFF_DELAY_MINUTES);
    }
    namespace server
    {
        constexpr uint16_t REFRESH_PERIOD_SECONDS = 60; /* Notify the server every X seconds */
    }
}
#endif