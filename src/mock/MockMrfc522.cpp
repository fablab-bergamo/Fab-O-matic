#include "mock/MockMrfc522.hpp"

namespace fablabbg
{
  MockMrfc522::UidDriver MockMrfc522::getDriverUid() const
  {
    UidDriver retVal;
    memset(&retVal, 0, sizeof(retVal));
    if (uid.has_value())
    {
      memcpy(retVal.uidByte, &uid.value(), sizeof(uid.value()));
      retVal.size = sizeof(uid.value());
      retVal.sak = 1;
    }
    return retVal;
  }

  MockMrfc522::MockMrfc522() : uid{std::nullopt} {};

  bool MockMrfc522::PICC_IsNewCardPresent() { return uid.has_value(); }

  bool MockMrfc522::PICC_ReadCardSerial() { return uid.has_value(); }

  void MockMrfc522::reset() { uid = std::nullopt; }

  bool MockMrfc522::PCD_Init() { return true; }

  bool MockMrfc522::PICC_WakeupA(byte *bufferATQA, byte &bufferSize)
  {
    if (uid.has_value())
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  bool MockMrfc522::PCD_PerformSelfTest() { return true; }

  void MockMrfc522::PCD_SetAntennaGain(MFRC522Constants::PCD_RxGain gain) {}

  void MockMrfc522::PCD_DumpVersionToSerial() {}

  void MockMrfc522::setUid(const std::optional<card::uid_t> &uid) { this->uid = uid; }

  void MockMrfc522::resetUid() { uid = std::nullopt; }
}