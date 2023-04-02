#ifndef _PINS_H_
#define _PINS_H_

#include <cstdint>

namespace pins
{
  namespace mfrc522
  {
    constexpr uint8_t ss_pin = 5U;
    constexpr uint8_t mosi_pin = 11U;
    constexpr uint8_t miso_pin = 12U;
    constexpr uint8_t sck_pin = 13U;
    constexpr uint8_t reset_pin = 4U;
  }
  namespace lcd
  {
    constexpr uint8_t rs_pin = 13U;
    constexpr uint8_t en_pin = 12U;
    constexpr uint8_t d0_pin = 14U;
    constexpr uint8_t d1_pin = 27U;
    constexpr uint8_t d2_pin = 26U;
    constexpr uint8_t d3_pin = 25U;
    constexpr uint8_t bl_pin = 9U;
  }
  namespace relay
  {
    constexpr uint8_t ch1_pin = 2U;
    constexpr uint8_t ch2_pin = 4U;
  }
}
#endif