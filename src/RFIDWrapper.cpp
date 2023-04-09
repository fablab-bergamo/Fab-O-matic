#include "RFIDWrapper.h"
#include "pins.h"
#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"

namespace Board
{
    // Only main.cpp instanciates the variables through Board.h file
    extern MFRC522 mfrc522;
}

RFIDWrapper::RFIDWrapper() {
    SPI.begin(pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin, pins.mfrc522.sda_pin);
}

bool RFIDWrapper::IsNewCardPresent()
{
    return Board::mfrc522.PICC_IsNewCardPresent();
}

bool RFIDWrapper::ReadCardSerial()
{
    return Board::mfrc522.PICC_ReadCardSerial();
}

void RFIDWrapper::SetUid(byte *arr)
{
    if (arr)
        memcpy(arr, Board::mfrc522.uid.uidByte, 10);
}

void RFIDWrapper::init()
{
    Board::mfrc522.PCD_Init();
    delay(10);
    MFRC522Debug::PCD_DumpVersionToSerial(Board::mfrc522, Serial);
    delay(10);
    char buffer[80]={0};
    sprintf(buffer, "Configured SPI RFID (SCK=%d, MISO=%d, MOSI=%d, SDA=%d)", pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin, pins.mfrc522.sda_pin);
    Serial.println(buffer);
    Board::mfrc522.PCD_SetAntennaGain(MFRC522::PCD_RxGain::RxGain_max);
    delay(10);
    if (!Board::mfrc522.PCD_PerformSelfTest())
        Serial.println("Self-test failure for RFID");
}

std::string RFIDWrapper::dumpUid()
{
    std::string output;
    auto uid = Board::mfrc522.uid.uidByte;
    for(auto i=0; i<sizeof(uid); i++)
    {
        char hexCar[2];
        sprintf(hexCar, "%02X", uid[i]);
        output.append(hexCar);
    }
    return output;
}
