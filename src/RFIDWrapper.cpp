#include "RFIDWrapper.h"
#include "pins.h"
#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"
#include "conf.h"
#include "card.h"

namespace Board
{
    // Only main.cpp instanciates the variables through Board.h file
    extern MFRC522 mfrc522;
}

RFIDWrapper::RFIDWrapper()
{
    SPI.begin(pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin, pins.mfrc522.sda_pin);
}

bool RFIDWrapper::IsNewCardPresent() const
{
    return Board::mfrc522.PICC_IsNewCardPresent();
}

bool RFIDWrapper::ReadCardSerial() const
{
    return Board::mfrc522.PICC_ReadCardSerial();
}

/// @brief Transforms the RFID acquired bytes into a uid_id object
/// @return card ID
card::uid_t RFIDWrapper::GetUid() const
{
    uint8_t arr[conf::whitelist::UID_BYTE_LEN];
    memcpy(arr, Board::mfrc522.uid.uidByte, conf::whitelist::UID_BYTE_LEN);
    return card::from_array(arr);
}

/// @brief Initializes RFID chip including self test
bool RFIDWrapper::init()
{
    char buffer[80] = {0};
    sprintf(buffer, "Configuring SPI RFID (SCK=%d, MISO=%d, MOSI=%d, SDA=%d)", pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin, pins.mfrc522.sda_pin);
    Serial.println(buffer);

    if (!Board::mfrc522.PCD_Init())
    {
        Serial.println("mfrc522 Init failed");
        return false;
    }

    MFRC522Debug::PCD_DumpVersionToSerial(Board::mfrc522, Serial);
    Board::mfrc522.PCD_SetAntennaGain(MFRC522::PCD_RxGain::RxGain_max);

    if (!Board::mfrc522.PCD_PerformSelfTest())
    {
        Serial.println("Self-test failure for RFID");
        return false;
    }
    return true;
}