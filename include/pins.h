#ifndef _PINS_H_
#define _PINS_H_

#include <cstdint>

namespace pins
{
#ifdef PINS_ESP32
  namespace mfrc522 /* RFID reader SPI interface pins definition */
  {
    constexpr uint8_t ss_pin = 5U; 
    constexpr uint8_t mosi_pin = 11U;
    constexpr uint8_t miso_pin = 12U;
    constexpr uint8_t sck_pin = 13U;
    constexpr uint8_t reset_pin = 4U;
  }
  namespace lcd /* LCD parallel interface pins definition */
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
    constexpr uint8_t ch1_pin = 2U; /* Control pin for Machine 1 */
    constexpr uint8_t ch2_pin = 4U; /* unclear */
  }
#endif
#ifdef PINS_ESP32S3
  namespace mfrc522 /* RFID reader SPI interface pins definition */
  {
    constexpr uint8_t ss_pin = 10U; 
    constexpr uint8_t mosi_pin = 11U;
    constexpr uint8_t miso_pin = 13U;
    constexpr uint8_t sck_pin = 12U;
    constexpr uint8_t reset_pin = 4U;
  }
  namespace lcd /* LCD parallel interface pins definition */
  {
    constexpr uint8_t rs_pin = 4U;
    constexpr uint8_t en_pin = 5U;
    constexpr uint8_t d0_pin = 6U;
    constexpr uint8_t d1_pin = 7U;
    constexpr uint8_t d2_pin = 8U;
    constexpr uint8_t d3_pin = 9U;
    constexpr uint8_t bl_pin = 10U;
  }
  namespace relay
  {
    constexpr uint8_t ch1_pin = 2U; /* Control pin for Machine 1 */
    constexpr uint8_t ch2_pin = 3U; /* unclear */
  }
#endif
}
#endif