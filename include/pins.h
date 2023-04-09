#ifndef _PINS_H_
#define _PINS_H_

#include <cstdint>
struct pins_config
{
  struct mfrc522_config
  { /* SPI RFID chip pins definition */
    uint8_t cs_pin;
    uint8_t mosi_pin;
    uint8_t miso_pin;
    uint8_t sck_pin;
    uint8_t reset_pin;
  };
  struct lcd_config /* LCD parallel interface pins definition */
  {
    uint8_t rs_pin;
    uint8_t en_pin;
    uint8_t d0_pin;
    uint8_t d1_pin;
    uint8_t d2_pin;
    uint8_t d3_pin;
    uint8_t bl_pin;
  };
  struct relay_config
  {
    uint8_t ch1_pin; /* Control pin for Machine 1 */
    uint8_t ch2_pin; /* unclear */
  };

  // Struct members
  mfrc522_config mfrc522;
  lcd_config lcd;
  relay_config relay;
};

#ifdef PINS_ESP32
constexpr pins_config pins{{5U, 11U, 12U, 13U, 4U}, {13U, 12U, 14U, 27U, 26U, 25U, 9U}, {2U, 4U}};
#endif
#ifdef PINS_ESP32S3
constexpr pins_config pins{{42U, 40U, 41U, 39U, 10U}, {10U, 5U, 6U, 7U, 8U, 9U, 12U}, {2U, 3U}};
#endif
#endif