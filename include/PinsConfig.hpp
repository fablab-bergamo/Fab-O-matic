#ifndef PINSCONFIG_HPP_
#define PINSCONFIG_HPP_

#include <cstdint>
#include <array>

namespace fabomatic
{
  /// @brief Constant used to indicate the pin is not used
  static constexpr uint8_t NO_PIN = -1;

  struct pins_config
  {
    /// @brief SPI RFID chip pins definition
    struct mfrc522_config
    {
      uint8_t sda_pin;
      uint8_t mosi_pin;
      uint8_t miso_pin;
      uint8_t sck_pin;
      uint8_t reset_pin;
    };

    /// @brief LCD parallel interface pins definition
    struct lcd_config
    {
      /// @brief reset pin
      uint8_t rs_pin;
      /// @brief enable pin
      uint8_t en_pin;
      uint8_t d0_pin;
      uint8_t d1_pin;
      uint8_t d2_pin;
      uint8_t d3_pin;
      /// @brief Backlight pin
      uint8_t bl_pin;
      /// @brief Backlight active low
      bool active_low;
    };
    /// @brief Hardwired relay configuration
    struct relay_config
    {
      /// @brief Control pin for Machine 1
      uint8_t ch1_pin;
      bool active_low;
    };
    /// @brief Configuration of the buzzer
    struct buzzer_config
    {
      uint8_t pin;
    };
    /// @brief Configuration of the LED/NeoPixel/RGB led
    struct led_config
    {
      uint8_t pin;
      bool is_neopixel;
      uint32_t neopixel_config;
      bool is_rgb;
      uint8_t green_pin;
      uint8_t blue_pin;
    };
    /// @brief Physical buttons wired to the ESP32
    struct buttons_config
    {
      uint8_t factory_defaults_pin;
    };

    mfrc522_config mfrc522;
    lcd_config lcd;
    relay_config relay;
    buzzer_config buzzer;
    led_config led;
    buttons_config buttons;
  };

  /// @brief Check at compile time that there are no duplicate pin definitions
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

    // C++20 provides constexpr sorting
    std::sort(pin_nums.begin(), pin_nums.end());

    uint8_t previous = NO_PIN;
    for (const auto pin : pin_nums)
    {
      if (pin == NO_PIN)
      {
        continue;
      }

      if (pin == previous)
      {
        return false;
      }

      previous = pin;

      // Check pins numbers are convertible to gpio_num_t
      [[maybe_unused]] auto test = static_cast<gpio_num_t>(pin);
    }

    return true;
  }

} // namespace fabomatic
#endif // PINSCONFIG_HPP_