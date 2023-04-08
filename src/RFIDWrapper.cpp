#include "RFIDWrapper.h"
#include "pins.h"
#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"

RFIDWrapper::RFIDWrapper(uint8_t cs_pin_num) {
  MFRC522DriverPinSimple cs_pin(cs_pin_num);
  MFRC522DriverSPI driver{cs_pin}; // Create SPI driver.
  MFRC522 mfrc522{driver}; // Create MFRC522 instance.
  this->mfrc522 = &mfrc522;
}

MFRC522 RFIDWrapper::get() {
    return *this->mfrc522;
}

bool RFIDWrapper::IsNewCardPresent() {
    return this->get().PICC_IsNewCardPresent();
}

bool RFIDWrapper::ReadCardSerial() {
    return this->get().PICC_ReadCardSerial();
}

void RFIDWrapper::SetUid(byte* arr) {
    if (arr)
        memcpy(arr, this->get().uid.uidByte, 10);
}

void RFIDWrapper::init() {
    this->get().PCD_Init();
}
