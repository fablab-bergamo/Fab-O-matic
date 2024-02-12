#include "Led.hpp"
#include "Logging.hpp"

namespace fablabbg
{
  void Led::init()
  {
    if (initialized || pins.led.pin == NO_PIN)
    {
      return;
    }

    ESP_LOGD(TAG, "Initializing LED (pin %d, is_neopixel %d, flags %u)",
             pins.led.pin, pins.led.is_neopixel, pins.led.neopixel_config);

    if (pins.led.is_neopixel)
    {
      pixel.begin();
    }
    else
    {
      digitalWrite(pins.led.pin, LOW);
      pinMode(pins.led.pin, OUTPUT);
    }
    initialized = true;
  }

  void Led::setColor(uint8_t r, uint8_t g, uint8_t b)
  {
    color = {r, g, b};
  }

  void Led::set(Status new_status)
  {
    this->status = new_status;
  }

  void Led::update()
  {
    init();

    switch (status)
    {
    case Status::OFF:
      pixel.setPixelColor(0, 0, 0, 0);
      isOn = false;
      break;
    case Status::ON:
      pixel.setPixelColor(0, color[0], color[1], color[2]);
      isOn = true;
      break;
    case Status::BLINK:
      if (isOn)
      {
        pixel.setPixelColor(0, color[0], color[1], color[2]);
      }
      else
      {
        pixel.setPixelColor(0, 0, 0, 0);
      }
      isOn = !isOn;
      break;
    }

    if (pins.led.is_neopixel)
    {
      pixel.show();
    }
    else
    {
      digitalWrite(pins.led.pin, isOn ? HIGH : LOW);
    }
  }

} // namespace fablabbg