#ifndef BASE_RFID_WRAPPER_HPP_
#define BASE_RFID_WRAPPER_HPP_
#include <optional>

#include "card.hpp"

namespace fablabbg
{
  class BaseRFIDWrapper
  {
  public:
    virtual bool init_rfid() const = 0;
    virtual bool isNewCardPresent() const = 0;
    virtual bool cardStillThere(const card::uid_t original, milliseconds delay) const = 0;
    virtual std::optional<card::uid_t> readCardSerial() const = 0;
    virtual bool selfTest() const = 0;
    virtual void reset() const = 0;
    virtual card::uid_t getUid() const = 0;
  };
} // namespace fablabbg
#endif // BASE_RFID_WRAPPER_HPP_