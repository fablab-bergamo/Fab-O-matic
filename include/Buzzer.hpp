#ifndef BUZZER_HPP_
#define BUZZER_HPP_

#include <inttypes.h>

namespace fabomatic
{
  /**
   * Represents the buzzer for BoardLogic
   */
  class Buzzer
  {
  private:
    mutable uint16_t beepCount{0};

  public:
    auto configure() -> void;
    auto beepOk() const -> void;
    auto beepFail() const -> void;
    // For testing purposes
    auto getBeepCount() const -> uint16_t;
  };
} // namespace fabomatic

#endif // BUZZER_HPP_