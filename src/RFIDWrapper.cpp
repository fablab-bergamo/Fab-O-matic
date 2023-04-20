#include "RFIDWrapper.h"
#include "pins.h"
#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"
#include "conf.h"
#include "card.h"
#include <memory>

RFIDWrapper::RFIDWrapper()
{
    // Configure SPI bus
    SPI.begin(pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin, pins.mfrc522.sda_pin);

    // Smart pointers members destructors will run & free the memory, when class will be distructed.
    this->rfid_simple_driver = std::unique_ptr<MFRC522DriverPinSimple>(new MFRC522DriverPinSimple(pins.mfrc522.sda_pin));
    this->spi_rfid_driver = std::unique_ptr<MFRC522DriverSPI>(new MFRC522DriverSPI(*this->rfid_simple_driver)); // Create SPI driver.
    this->mfrc522 = std::unique_ptr<MFRC522>(new MFRC522(*this->spi_rfid_driver));
}

bool RFIDWrapper::isNewCardPresent() const
{
    return this->mfrc522->PICC_IsNewCardPresent();
}

bool RFIDWrapper::readCardSerial() const
{
    return this->mfrc522->PICC_ReadCardSerial();
}

/// @brief Transforms the RFID acquired bytes into a uid_id object
/// @return card ID
card::uid_t RFIDWrapper::getUid() const
{
    uint8_t arr[conf::whitelist::UID_BYTE_LEN];
    memcpy(arr, this->mfrc522->uid.uidByte, conf::whitelist::UID_BYTE_LEN);
    return card::from_array(arr);
}

/// @brief Initializes RFID chip including self test
bool RFIDWrapper::init() const
{
    char buffer[80] = {0};
    sprintf(buffer, "Configuring SPI RFID (SCK=%d, MISO=%d, MOSI=%d, SDA=%d)", pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin, pins.mfrc522.sda_pin);
    Serial.println(buffer);

    if (!this->mfrc522->PCD_Init())
    {
        Serial.println("mfrc522 Init failed");
        return false;
    }

    MFRC522Debug::PCD_DumpVersionToSerial(*this->mfrc522, Serial);
    this->mfrc522->PCD_SetAntennaGain(MFRC522::PCD_RxGain::RxGain_max);

    if (!this->mfrc522->PCD_PerformSelfTest())
    {
        Serial.println("Self-test failure for RFID");
        return false;
    }
    return true;
}