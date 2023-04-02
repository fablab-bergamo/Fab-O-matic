#ifndef _CONF_H_
#define _CONF_H_

#include <cstdint>

namespace conf
{
    namespace whitelist
    {
        constexpr uint8_t LEN = 10U;
    }
    namespace lcd
    {
        constexpr uint8_t ROWS = 2U;
        constexpr uint8_t COLS = 16U;
    }
    namespace machine
    {
        constexpr uint8_t CONTROL_PIN_1 = 2U;
    }
}
#endif