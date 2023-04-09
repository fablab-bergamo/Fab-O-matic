#ifndef _RFID_WRAPPER_H_
#define _RFID_WRAPPER_H_

#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"

class RFIDWrapper
{
public:
    RFIDWrapper();
    void init();
    bool IsNewCardPresent();
    bool ReadCardSerial();
    void SetUid(byte *arr);
};

#endif