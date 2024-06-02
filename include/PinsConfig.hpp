#ifndef PINSCONFIG_HPP_
#define PINSCONFIG_HPP_

#include <cstdint>
#include <array>
#include <Adafruit_NeoPixel.h>

namespace fabomatic
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
      bool active_low;
    };
    struct buzzer_config
    {
      uint8_t pin;
    };
    struct led_config
    {
      uint8_t pin;
      bool is_neopixel;
      uint32_t neopixel_config;
      bool is_rgb;
      uint8_t green_pin;
      uint8_t blue_pin;
    };
    struct buttons_config
    {
      uint8_t factory_defaults_pin;
    };
    // Struct members
    mfrc522_config mfrc522;
    lcd_config lcd;
    relay_config relay;
    buzzer_config buzzer;
    led_config led;
    buttons_config buttons;
  };

  // Check at compile time that there are no duplicate pin definitions
  constexpr bool no_duplicates(pins_config pins)
  {
    std::array pin_nums{
        pins.mfrc522.sda_pin,
        pins.mfrc522.mosi_pin,
        pins.mfrc522.miso_pin,
        pins.mfrc522.sck_pin,
        pins.mfrc522.reset_pin,
        pins.lcd.rs_pin,
        pins.lcd.en_pin,
        pins.lcd.d0_pin,
        pins.lcd.d1_pin,
        pins.lcd.d2_pin,
        pins.lcd.d3_pin,
        pins.lcd.bl_pin,
        pins.relay.ch1_pin,
        pins.buzzer.pin,
        pins.led.pin,
        pins.led.green_pin,
        pins.led.blue_pin,
        pins.buttons.factory_defaults_pin};

    // No constexpr std::sort available
    for (auto i = 0; i < pin_nums.size(); ++i)
    {
      if (pin_nums[i] == NO_PIN)
        continue;

      for (auto j = i + 1; j < pin_nums.size(); ++j)
      {
        if (pin_nums[i] == pin_nums[j])
          return false;
      }
      // Check pins numbers are convertible to gpio_num_t
      static_cast<gpio_num_t>(pin_nums[i]);
    }
    return true;
  }
  
} // namespace fabomatic
#endif // PINSCONFIG_HPP_