#include "mock/MockMrfc522.hpp"

namespace fablabbg
{
  MockMrfc522::UidDriver MockMrfc522::getDriverUid() const
  {
    UidDriver retVal;
    memset(&retVal, 0, sizeof(retVal));
    if (getSimulatedUid().has_value())
    {
      memcpy(retVal.uidByte, &uid.value(), sizeof(uid.value()));
      retVal.size = sizeof(uid.value());
      retVal.sak = 1;
    }
    return retVal;
  }

  bool MockMrfc522::PICC_IsNewCardPresent() { return getSimulatedUid().has_value(); }

  bool MockMrfc522::PICC_ReadCardSerial() { return getSimulatedUid().has_value(); }

  void MockMrfc522::reset() { uid = std::nullopt; }

  bool MockMrfc522::PCD_Init() { return true; }

  bool MockMrfc522::PICC_WakeupA(byte *bufferATQA, byte &bufferSize)
  {
    if (getSimulatedUid().has_value())
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

  void MockMrfc522::setUid(const std::optional<card::uid_t> &uid, const std::optional<milliseconds> &max_delay)
  {
    this->uid = uid;
    if (max_delay.has_value())
    {
      stop_uid_simulate_time = std::chrono::system_clock::now() + max_delay.value();
    }
    else
    {
      stop_uid_simulate_time = std::nullopt;
    }
  }

  void MockMrfc522::resetUid()
  {
    uid = std::nullopt;
    stop_uid_simulate_time = std::nullopt;
  }

  std::optional<card::uid_t> MockMrfc522::getSimulatedUid() const
  {
    if (stop_uid_simulate_time.has_value() && std::chrono::system_clock::now() > stop_uid_simulate_time.value())
    {
      return std::nullopt;
    }
    else
    {
      return uid;
    }
  }
}