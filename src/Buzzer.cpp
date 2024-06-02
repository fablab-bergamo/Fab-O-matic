#include "Buzzer.hpp"
#include "conf.hpp"
#include "pins.hpp"
#include "Arduino.h"
#include "Tasks.hpp"

namespace fabomatic
{
  auto Buzzer::configure() -> void
  {
    if constexpr (pins.buzzer.pin != NO_PIN)
    {
      pinMode(pins.buzzer.pin, OUTPUT);
    }
  }
  auto Buzzer::beepOk() const -> void
  {
    if constexpr (conf::buzzer::STANDARD_BEEP_DURATION > 0ms && pins.buzzer.pin != NO_PIN)
    {
      digitalWrite(pins.buzzer.pin, 1);
      Tasks::delay(conf::buzzer::STANDARD_BEEP_DURATION);
      digitalWrite(pins.buzzer.pin, 0);
    }
    beepCount++;
  }

  auto Buzzer::beepFail() const -> void
  {
    if constexpr (conf::buzzer::STANDARD_BEEP_DURATION > 0ms && pins.buzzer.pin != NO_PIN)
    {
      for (auto i = 0; i < conf::buzzer::NB_BEEPS; i++)
      {
        digitalWrite(pins.buzzer.pin, 1);
        Tasks::delay(conf::buzzer::STANDARD_BEEP_DURATION);
        digitalWrite(pins.buzzer.pin, 0);
        Tasks::delay(conf::buzzer::STANDARD_BEEP_DURATION);
        beepCount++;
      }
    }
  }

  auto Buzzer::getBeepCount() const -> uint16_t
  {
    return beepCount;
  }

} // namespace fabomatic