#include "Arduino.h"
#include <memory>

#include "MFRC522DriverPinSimple.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522v2.h"

#include "Mrfc522Driver.hpp"

namespace fablabbg
{
  Mrfc522Driver::Mrfc522Driver() : rfid_simple_driver(std::make_unique<MFRC522DriverPinSimple>(pins.mfrc522.sda_pin)),
                                   spi_rfid_driver(std::make_unique<MFRC522DriverSPI>(*rfid_simple_driver)),
                                   mfrc522(std::make_unique<MFRC522>(*spi_rfid_driver))
  {
    // Configure SPI bus
    SPI.begin(pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin, pins.mfrc522.sda_pin);
    pinMode(pins.mfrc522.reset_pin, OUTPUT);
  };

  bool Mrfc522Driver::PICC_IsNewCardPresent() { return mfrc522->PICC_IsNewCardPresent(); }

  bool Mrfc522Driver::PICC_ReadCardSerial() { return mfrc522->PICC_ReadCardSerial(); }

  void Mrfc522Driver::reset() { mfrc522->PCD_Reset(); }

  bool Mrfc522Driver::PCD_Init()
  {
    return mfrc522->PCD_Init();
  }

  bool Mrfc522Driver::PICC_WakeupA(byte *bufferATQA, byte bufferSize) { return mfrc522->PICC_WakeupA(bufferATQA, &bufferSize) == MFRC522::StatusCode::STATUS_OK; }

  bool Mrfc522Driver::PCD_PerformSelfTest() { return mfrc522->PCD_PerformSelfTest(); }

  Mrfc522Driver::UidDriver Mrfc522Driver::getDriverUid() const
  {
    UidDriver retVal{};
    memset(&retVal, 0, sizeof(retVal));
    memcpy(retVal.uidByte, mfrc522->uid.uidByte, sizeof(retVal.uidByte));
    retVal.size = mfrc522->uid.size;
    retVal.sak = mfrc522->uid.sak;
    return retVal;
  }
  void Mrfc522Driver::PCD_SetAntennaGain(MFRC522Constants::PCD_RxGain gain) { mfrc522->PCD_SetAntennaGain(gain); }

  void Mrfc522Driver::PCD_DumpVersionToSerial() { MFRC522Debug::PCD_DumpVersionToSerial(*mfrc522, Serial); }

} // namespace fablabbg