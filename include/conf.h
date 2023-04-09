#ifndef _CONF_H_
#define _CONF_H_

#include <cstdint>

namespace conf
{
    namespace whitelist
    {
        constexpr uint8_t LEN = 10U; /* Maximum number of whitelisted cards */
    }
    namespace lcd
    {
        constexpr uint8_t ROWS = 2U;  /* Number of rows for LCD */
        constexpr uint8_t COLS = 16U; /* Number of cols for LCD */
    }
}
#endif