#include "mock/MockMrfc522.hpp"
#include "Tasks.hpp"

#include <algorithm>

namespace fabomatic
{
  auto MockMrfc522::getDriverUid() const -> MockMrfc522::UidDriver
  {
    UidDriver retVal{};
    if (getSimulatedUid().has_value())
    {
      retVal.size = sizeof(uid.value());
      const auto arr_uid = card::to_array(uid.value());
      std::copy(arr_uid.begin(), arr_uid.end(), retVal.uidByte.begin());
      retVal.sak = 1;
    }
    return retVal;
  }

  auto MockMrfc522::PICC_IsNewCardPresent() -> bool { return getSimulatedUid().has_value(); }

  auto MockMrfc522::PICC_ReadCardSerial() -> bool { return getSimulatedUid().has_value(); }

  void MockMrfc522::reset() { uid = std::nullopt; }

  auto MockMrfc522::PCD_Init() -> bool { return true; }

  auto MockMrfc522::PICC_WakeupA(byte *bufferATQA, byte &bufferSize) -> bool
  {
    if (getSimulatedUid().has_value())
    {
      return true;
    }
    return false;
  }

  auto MockMrfc522::PCD_PerformSelfTest() -> bool { return true; }

  auto MockMrfc522::PCD_SetAntennaGain(MFRC522Constants::PCD_RxGain gain) -> void {}

  auto MockMrfc522::PCD_DumpVersionToSerial() -> void {}

  auto MockMrfc522::setUid(const std::optional<card::uid_t> &uid, const std::optional<std::chrono::milliseconds> &max_delay) -> void
  {
    this->uid = uid;
    if (max_delay.has_value())
    {
      stop_uid_simulate_time = fabomatic::Tasks::arduinoNow() + max_delay.value();
    }
    else
    {
      stop_uid_simulate_time = std::nullopt;
    }
  }

  auto MockMrfc522::resetUid() -> void
  {
    uid = std::nullopt;
    stop_uid_simulate_time = std::nullopt;
  }

  auto MockMrfc522::getSimulatedUid() const -> std::optional<card::uid_t>
  {
    if (stop_uid_simulate_time.has_value() && fabomatic::Tasks::arduinoNow() > stop_uid_simulate_time.value())
    {
      return std::nullopt;
    }
    return uid;
  }
} // namespace fabomatic