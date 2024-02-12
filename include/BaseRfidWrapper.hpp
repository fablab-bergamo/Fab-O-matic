#ifndef BASERFIDWRAPPER_HPP_
#define BASERFIDWRAPPER_HPP_
#include <optional>

#include "card.hpp"

namespace fablabbg
{
  class BaseRFIDWrapper
  {
  public:
    virtual bool init_rfid() const = 0;
    [[nodiscard]] virtual bool isNewCardPresent() const = 0;
    [[nodiscard]] virtual bool cardStillThere(const card::uid_t original, std::chrono::milliseconds delay) const = 0;
    virtual std::optional<card::uid_t> readCardSerial() const = 0;
    virtual bool selfTest() const = 0;
    virtual void reset() const = 0;
    [[nodiscard]] virtual card::uid_t getUid() const = 0;
  };
} // namespace fablabbg
#endif // BASERFIDWRAPPER_HPP_