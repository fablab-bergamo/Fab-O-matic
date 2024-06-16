#ifndef PINS_H_
#define PINS_H_

#include <cstdint>
#include <Adafruit_NeoPixel.h>
#include <PinsConfig.hpp>

namespace fabomatic
{
  /// @brief Returns configuration for an ESP32 dev board used in revision 0.1
  /// @return pins_config
  constexpr auto configure_pins_esp32() -> const pins_config
  {
    return pins_config{
        {
            .sda_pin = 27U,
            .mosi_pin = 33U,
            .miso_pin = 32U,
            .sck_pin = 26U,
            .reset_pin = 20U,
        }, // RFID
        {
            .rs_pin = 15U,
            .en_pin = 2U,
            .d0_pin = 0U,
            .d1_pin = 4U,
            .d2_pin = 16U,
            .d3_pin = 17U,
            .bl_pin = 18U,
            .active_low = false,
        }, // LCD
        {
            .ch1_pin = 14U,
            .active_low = true,
        }, // relay
        {
            .pin = 12U,
        }, // buzzer
        {
            .pin = 19U,
            .is_neopixel = true,
            .neopixel_config = NEO_RGB + NEO_KHZ800,
            .is_rgb = false,
            .green_pin = NO_PIN,
            .blue_pin = NO_PIN,
        }, // LED config
        {
            .factory_defaults_pin = NO_PIN,
        } // Factory defaults button
    };
  }

  /// @brief Returns configuration for Wokwi ESP32S2 environment
  /// @return pins_config
  constexpr auto configure_pins_wokwi() -> const pins_config
  {
    return pins_config{
        {
            .sda_pin = 27U,
            .mosi_pin = 26U,
            .miso_pin = 33U,
            .sck_pin = 32U,
            .reset_pin = 16U,
        }, // RFID
        {
            .rs_pin = 15U,
            .en_pin = 18U,
            .d0_pin = 2U,
            .d1_pin = 4U,
            .d2_pin = 5U,
            .d3_pin = 19U,
            .bl_pin = NO_PIN,
            .active_low = false,
        }, // LCD
        {
            .ch1_pin = 14U,
            .active_low = false,
        }, // relay
        {
            .pin = 12U,
        }, // buzzer
        {
            .pin = 20U,
            .is_neopixel = true,
            .neopixel_config = NEO_GRB + NEO_KHZ800,
            .is_rgb = false,
            .green_pin = NO_PIN,
            .blue_pin = NO_PIN,
        }, // LED config
        {
            .factory_defaults_pin = 21U,
        } // Factory defaults button
    };
  }

  /// @brief Returns configuration for esp32s3 dev kit board
  /// @return pins_config
  constexpr auto configure_pins_esp32s3() -> const pins_config
  {
    return pins_config{
        {
            .sda_pin = 17U,
            .mosi_pin = 8U,
            .miso_pin = 3U,
            .sck_pin = 18U,
            .reset_pin = 12U,
        }, // RFID
        {
            .rs_pin = 5U,
            .en_pin = 4U,
            .d0_pin = 6U,
            .d1_pin = 7U,
            .d2_pin = 15U,
            .d3_pin = 2U,
            .bl_pin = 13U,
            .active_low = false,
        }, // LCD
        {
            .ch1_pin = 10U,
            .active_low = true,
        }, // relay
        {
            .pin = 9U,
        }, // buzzer
        {
            .pin = 48U,
            .is_neopixel = true,
            .neopixel_config = NEO_GRB + NEO_KHZ800,
            .is_rgb = false,
            .green_pin = NO_PIN,
            .blue_pin = NO_PIN,
        }, // LED config
        {
            .factory_defaults_pin = NO_PIN,
        } // Factory defaults button
    };
  }

  /// @brief Returns configuration for PCB hardware in this repository
  /// @return pins_config
  constexpr auto configure_pins_hwrev0() -> const pins_config
  {
    return pins_config{
        {
            .sda_pin = 39U,
            .mosi_pin = 37U,
            .miso_pin = 36U,
            .sck_pin = 38U,
            .reset_pin = 35U,
        }, // RFID
        {
            .rs_pin = 13U,
            .en_pin = 14U,
            .d0_pin = 21U,
            .d1_pin = 47U,
            .d2_pin = 48U,
            .d3_pin = 40U,
            .bl_pin = NO_PIN,
            .active_low = false,
        }, // LCD
        {
            .ch1_pin = 10U,
            .active_low = true,
        }, // relay
        {
            .pin = 15U,
        }, // buzzer
        {
            .pin = 18U,
            .is_neopixel = true,
            .neopixel_config = NEO_GRB + NEO_KHZ800,
            .is_rgb = false,
            .green_pin = NO_PIN,
            .blue_pin = NO_PIN,
        }, // LED config
        {
            .factory_defaults_pin = 8U,
        } // Factory defaults button
    };
  }

  /// @brief Configuration for ESP32 Wrover Kit 4.1 (https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-wrover-kit.html)
  /// @return pins_config
  constexpr auto configure_pins_wrover() -> const pins_config
  {
    return pins_config{
        {
            .sda_pin = 15U,
            .mosi_pin = 19U,
            .miso_pin = 16U,
            .sck_pin = 20U,
            .reset_pin = 18U,
        }, // RFID
        {
            .rs_pin = 12U,
            .en_pin = 14U,
            .d0_pin = 26U,
            .d1_pin = 21U,
            .d2_pin = 22U,
            .d3_pin = 23U,
            .bl_pin = NO_PIN,
            .active_low = false,
        }, // LCD
        {
            .ch1_pin = 5U,
            .active_low = false,
        }, // relay
        {
            .pin = 13U,
        }, // buzzer
        {
            .pin = 0U,
            .is_neopixel = false,
            .neopixel_config = 0,
            .is_rgb = true,
            .green_pin = 2U,
            .blue_pin = 4U,
        }, // LED configuration
        {
            .factory_defaults_pin = NO_PIN,
        } // Factory defaults button
    };
  }

  /// @brief Returns the actual pins configuration to be used depending on preprocessor constants
  /// @return pins_config
  constexpr auto configure_pins() -> const pins_config
  {
#ifdef PINS_ESP32
    return configure_pins_esp32();
#endif
#ifdef PINS_WOKWI
    return configure_pins_wokwi();
#endif
#ifdef PINS_ESP32S3
    return configure_pins_esp32s3();
#endif
#ifdef PINS_HARDWARE_REV0
    return configure_pins_hwrev0();
#endif
#ifdef PINS_ESP32_WROVERKIT
    return configure_pins_wrover();
#endif
  }

  static constexpr pins_config pins = configure_pins();

  static_assert(no_duplicates(pins), "Duplicate pin definition, check pins.hpp");
  static_assert(!(pins.led.is_neopixel && pins.led.is_rgb), "Neopixel and RGB led cannot be used at the same time");

} // namespace fabomatic
#endif // PINS_H_