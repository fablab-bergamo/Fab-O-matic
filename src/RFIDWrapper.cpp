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

RFIDWrapper::RFIDWrapper() {}

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
}
