#ifndef _BASE_RFID_WRAPPER_HPP_
#define _BASE_RFID_WRAPPER_HPP_
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

    // Testing methods
    virtual void resetUid() = 0;
    virtual void setUid(const card::uid_t &uid) = 0;
  };
}
#endif // _BASE_RFID_WRAPPER_HPP_