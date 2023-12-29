#ifndef _BASE_RFID_WRAPPER_HPP_
#define _BASE_RFID_WRAPPER_HPP_
#include <optional>

#include "card.hpp"

namespace fablabbg
{
  class BaseRFIDWrapper
  {
  public:
    virtual bool init_rfid() const;
    virtual bool isNewCardPresent() const;
    virtual bool cardStillThere(const card::uid_t original, milliseconds delay) const;
    virtual std::optional<card::uid_t> readCardSerial() const;
    virtual bool selfTest() const;
    virtual void reset() const;
    virtual card::uid_t getUid() const;
  };
}
#endif // _BASE_RFID_WRAPPER_HPP_