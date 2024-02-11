#ifndef LED_HPP_
#define LED_HPP_

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <memory>

#include "pins.hpp"

namespace fablabbg
{
  class Led
  {
  public:
    enum class Status : uint8_t
    {
      OFF,
      ON,
      BLINK,
    };

    Led() = default;

    void set(Status status);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void update();

  private:
    Adafruit_NeoPixel pixel{1, pins.led.pin, pins.led.neopixel_config};
    uint8_t color[3]{128, 128, 128};
    Status status{Status::ON};
    bool isOn{false};
    bool initialized{false};

    void init();
  };

} // namespace fablabbg

#endif // LED_HPP_