#ifndef _MRFC522_DRIVER_HPP_
#define _MRFC522_DRIVER_HPP_

#include <memory>
#include "Arduino.h"

#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"
#include "FabUser.hpp"
#include "pins.hpp"

namespace fablabbg
{
  class Mrfc522Driver
  {
  private:
    std::unique_ptr<MFRC522DriverPinSimple> rfid_simple_driver;
    std::unique_ptr<MFRC522DriverSPI> spi_rfid_driver;
    std::unique_ptr<MFRC522> mfrc522;

  public:
    struct UidDriver
    {
      byte size; // Number of bytes in the UID. 4, 7 or 10.
      byte uidByte[10];
      byte sak; // The SAK (Select acknowledge) byte returned from the PICC after successful selection.
    };

    Mrfc522Driver();
    bool PICC_IsNewCardPresent();
    bool PICC_ReadCardSerial();
    void reset();
    bool PCD_Init();
    bool PICC_WakeupA(byte *bufferATQA, byte bufferSize);
    bool PCD_PerformSelfTest();
    UidDriver getDriverUid() const;
    void PCD_SetAntennaGain(MFRC522Constants::PCD_RxGain gain);
    void PCD_DumpVersionToSerial();

    void setUid(const std::optional<card::uid_t> &uid);
    void resetUid();

    static constexpr auto RxGainMax = MFRC522::PCD_RxGain::RxGain_max;
  };
} // namespace fablabbg

#endif // _MRFC522_DRIVER_HPP_