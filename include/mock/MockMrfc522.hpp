#ifndef MOCK_MOCKMRFC522_HPP_
#define MOCK_MOCKMRFC522_HPP_

#include "Arduino.h"

#include <array>
#include <memory>
#include <optional>

#include "FabUser.hpp"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522v2.h"

namespace fabomatic
{
  /**
   * This class implements a Mrfc522 with settable tag ID in order to allow simulation
   */
  class MockMrfc522
  {
  private:
    std::optional<card::uid_t> uid{std::nullopt};
    std::optional<std::chrono::milliseconds> stop_uid_simulate_time{std::nullopt};
    std::optional<card::uid_t> getSimulatedUid() const;

  public:
    struct UidDriver
    {
      byte size{0}; // Number of bytes in the UID. 4, 7 or 10.
      std::array<byte, 10> uidByte{0};
      byte sak{0}; // The SAK (Select acknowledge) byte returned from the PICC after successful selection.
    };

    constexpr MockMrfc522() {};

    auto PICC_IsNewCardPresent() -> bool;
    auto PICC_ReadCardSerial() -> bool;
    auto reset() -> void;
    auto PCD_Init() -> bool;
    auto PICC_WakeupA(byte *bufferATQA, byte &bufferSize) -> bool;
    auto PCD_PerformSelfTest() -> bool;
    auto getDriverUid() const -> MockMrfc522::UidDriver;
    auto PCD_SetAntennaGain(MFRC522Constants::PCD_RxGain gain) -> void;
    auto PCD_DumpVersionToSerial() -> void;

    auto setUid(const std::optional<card::uid_t> &uid,
                const std::optional<std::chrono::milliseconds> &max_delay) -> void;
    auto resetUid() -> void;

    static constexpr auto RxGainMax = MFRC522::PCD_RxGain::RxGain_max;
  };
} // namespace fabomatic

#endif // MOCK_MOCKMRFC522_HPP_