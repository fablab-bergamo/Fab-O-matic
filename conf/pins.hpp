#ifndef PINS_H_
#define PINS_H_

#include <cstdint>

namespace fablabbg
{
  static constexpr uint8_t NO_PIN = -1;

  struct pins_config
  {
    struct mfrc522_config
    { /* SPI RFID chip pins definition */
      uint8_t sda_pin;
      uint8_t mosi_pin;
      uint8_t miso_pin;
      uint8_t sck_pin;
      uint8_t reset_pin;
    };
    struct lcd_config /* LCD parallel interface pins definition */
    {
      uint8_t rs_pin; /* Reset */
      uint8_t en_pin; /* Enable */
      uint8_t d0_pin;
      uint8_t d1_pin;
      uint8_t d2_pin;
      uint8_t d3_pin;
      uint8_t bl_pin;  /* Backlight pin */
      bool active_low; /* Backlight active low*/
    };
    struct relay_config
    {
      uint8_t ch1_pin; /* Control pin for Machine 1 */
      uint8_t ch2_pin; /* unclear */
    };
    struct buzzer_config
    {
      uint8_t pin;
    };
    struct led_config
    {
      uint8_t pin;
      bool is_neopixel;
    };
    // Struct members
    mfrc522_config mfrc522;
    lcd_config lcd;
    relay_config relay;
    buzzer_config buzzer;
    led_config led;
  };

#ifdef PINS_ESP32
  constexpr pins_config pins{
      {27U, 33U, 32U, 26U, 4U},                // RFID
      {15U, 2U, 0U, 4U, 16U, 17U, 18U, false}, // LCD
      {14U, 27U},                              // relay
      {12U},                                   // buzzer
      {19U, false}                             // Neopixel (non testato)
  };
#endif
#if (WOKWI_SIMULATION)
  static constexpr pins_config pins{
      {27U, 26U, 33U, 32U, 4U},               // RFID
      {15U, 18U, 2U, 4U, 5U, 19U, 9U, false}, // LCD
      {14U, 27U},                             // relay
      {12U},                                  // buzzer
      {18U, true}                             // Neopixel
  };
#endif
#ifdef PINS_ESP32S3
  static constexpr pins_config pins{
      {17U, 8U, 3U, 18U, 12U},               // RFID
      {5U, 4U, 6U, 7U, 15U, 2U, 13U, false}, // LCD
      {10U, 11U},                            // relay
      {9U},                                  // buzzer
      {48U, true}                            // Neopixel
  };
#endif
} // namespace fablabbg
#endif // PINS_H_