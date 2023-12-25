#ifndef PINS_H_
#define PINS_H_

#include <cstdint>

static constexpr uint8_t NO_PIN = -1;

struct pins_config
{
  struct mfrc522_config
  { /* SPI RFID chip pins definition */
    const uint8_t sda_pin;
    const uint8_t mosi_pin;
    const uint8_t miso_pin;
    const uint8_t sck_pin;
    const uint8_t reset_pin;
  };
  struct lcd_config /* LCD parallel interface pins definition */
  {
    const uint8_t rs_pin; /* Reset */
    const uint8_t en_pin; /* Enable */
    const uint8_t d0_pin;
    const uint8_t d1_pin;
    const uint8_t d2_pin;
    const uint8_t d3_pin;
    const uint8_t bl_pin;  /* Backlight pin */
    const bool active_low; /* Backlight active low*/
  };
  struct relay_config
  {
    const uint8_t ch1_pin; /* Control pin for Machine 1 */
    const uint8_t ch2_pin; /* unclear */
  };
  struct buzzer_config
  {
    const uint8_t pin;
  };
  struct led_config
  {
    const uint8_t pin;
    const bool is_neopixel;
  };
  // Struct members
  const mfrc522_config mfrc522;
  const lcd_config lcd;
  const relay_config relay;
  const buzzer_config buzzer;
  const led_config led;
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
#if(WOKWI_SIMULATION)
constexpr pins_config pins{
    {27U, 26U, 33U, 32U, 4U},               // RFID
    {15U, 18U, 2U, 4U, 5U, 19U, 9U, false}, // LCD
    {14U, 27U},                             // relay
    {12U},                                  // buzzer
    {18U, true}                             // Neopixel
};
#endif
#ifdef PINS_ESP32S3
constexpr pins_config pins{
    {17U, 8U, 3U, 18U, 12U},               // RFID
    {5U, 4U, 6U, 7U, 15U, 2U, 13U, false}, // LCD
    {10U, 11U},                            // relay
    {9U},                                  // buzzer
    {48U, true}                            // Neopixel
};
#endif
#endif // PINS_H_