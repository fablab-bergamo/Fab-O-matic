#ifndef MRFC522DRIVER_HPP_
#define MRFC522DRIVER_HPP_

#include "Arduino.h"
#include <memory>
#include <array>

#include "FabUser.hpp"
#include "MFRC522Debug.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522v2.h"
#include "pins.hpp"

namespace fabomatic
{
  /// @brief The real Mrfc522Driver around the Arduino library.
  class Mrfc522Driver
  {
  private:
    std::unique_ptr<MFRC522DriverPinSimple> rfid_simple_driver;
    std::unique_ptr<MFRC522DriverSPI> spi_rfid_driver;
    std::unique_ptr<MFRC522> mfrc522;
    auto hardReset() -> void;

  public:
    struct UidDriver
    {
      byte size{0};                    // Number of bytes in the UID. 4, 7 or 10.
      std::array<byte, 10> uidByte{0}; // The UID (Unique Identifier) of the PICC.
      byte sak{0};                     // The SAK (Select acknowledge) byte returned from the PICC after successful selection.
    };

    Mrfc522Driver();
    auto PICC_IsNewCardPresent() -> bool;
    auto PICC_ReadCardSerial() -> bool;
    auto reset() -> void;
    auto PCD_Init() -> bool;
    auto PICC_WakeupA(byte *bufferATQA, byte bufferSize) -> bool;
    auto PCD_PerformSelfTest() -> bool;
    auto getDriverUid() const -> UidDriver;
    auto PCD_SetAntennaGain(MFRC522Constants::PCD_RxGain gain) -> void;
    auto PCD_DumpVersionToSerial() -> void;

    static constexpr auto RxGainMax = MFRC522::PCD_RxGain::RxGain_max;
  };
} // namespace fabomatic

#endif // MRFC522DRIVER_HPP_