#include "Arduino.h"
#include <algorithm>
#include <memory>

#include "MFRC522DriverPinSimple.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522v2.h"

#include "Mrfc522Driver.hpp"

namespace fabomatic
{
  Mrfc522Driver::Mrfc522Driver()
  {
    hardReset();

    // Configure SPI bus
    SPI.end();
    SPI.begin(pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin, pins.mfrc522.sda_pin);

    rfid_simple_driver = std::make_unique<MFRC522DriverPinSimple>(pins.mfrc522.sda_pin);
    spi_rfid_driver = std::make_unique<MFRC522DriverSPI>(*rfid_simple_driver, SPI);
    mfrc522 = std::make_unique<MFRC522>(*spi_rfid_driver);
  };

  auto Mrfc522Driver::hardReset() -> void
  {
    // First set the resetPowerDownPin as digital input, to check the MFRC522 power down mode.
    pinMode(pins.mfrc522.reset_pin, INPUT);

    if (digitalRead(pins.mfrc522.reset_pin) == LOW)
    {                                             // The MFRC522 chip is in power down mode.
      pinMode(pins.mfrc522.reset_pin, OUTPUT);    // Now set the resetPowerDownPin as digital output.
      digitalWrite(pins.mfrc522.reset_pin, LOW);  // Make sure we have a clean LOW state.
      delayMicroseconds(2);                       // 8.8.1 Reset timing requirements says about 100ns. Let us be generous: 2μsl
      digitalWrite(pins.mfrc522.reset_pin, HIGH); // Exit power down mode. This triggers a hard reset.
      // Section 8.8.2 in the datasheet says the oscillator start-up time is the start up time of the crystal + 37,74μs. Let us be generous: 50ms.
      delay(50);
    }
  }

  bool Mrfc522Driver::PICC_IsNewCardPresent() { return mfrc522->PICC_IsNewCardPresent(); }

  bool Mrfc522Driver::PICC_ReadCardSerial() { return mfrc522->PICC_ReadCardSerial(); }

  void Mrfc522Driver::reset()
  {
    hardReset();
    mfrc522->PCD_Reset();
  }

  bool Mrfc522Driver::PCD_Init()
  {
    return mfrc522->PCD_Init();
  }

  bool Mrfc522Driver::PICC_WakeupA(byte *bufferATQA, byte bufferSize) { return mfrc522->PICC_WakeupA(bufferATQA, &bufferSize) == MFRC522::StatusCode::STATUS_OK; }

  bool Mrfc522Driver::PCD_PerformSelfTest() { return mfrc522->PCD_PerformSelfTest(); }

  auto Mrfc522Driver::getDriverUid() const -> UidDriver
  {
    UidDriver retVal{};
    std::copy(mfrc522->uid.uidByte, mfrc522->uid.uidByte + 10, retVal.uidByte.begin());
    retVal.size = mfrc522->uid.size;
    retVal.sak = mfrc522->uid.sak;
    return retVal;
  }
  void Mrfc522Driver::PCD_SetAntennaGain(MFRC522Constants::PCD_RxGain gain) { mfrc522->PCD_SetAntennaGain(gain); }

  void Mrfc522Driver::PCD_DumpVersionToSerial() { MFRC522Debug::PCD_DumpVersionToSerial(*mfrc522, Serial); }

} // namespace fabomatic