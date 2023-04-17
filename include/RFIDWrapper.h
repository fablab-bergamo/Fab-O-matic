#ifndef RFIDWRAPPER_H_
#define RFIDWRAPPER_H_

#include "conf.h"
#include <string>
#include "card.h"
#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"

class RFIDWrapper
{
private:
    MFRC522 *mfrc522;
    MFRC522DriverPinSimple *rfid_simple_driver;
    MFRC522DriverSPI *spi_rfid_driver;

public:
    RFIDWrapper();
    ~RFIDWrapper();

    bool init() const;
    bool isNewCardPresent() const;
    bool readCardSerial() const;
    card::uid_t getUid() const;
};

#endif // RFIDWRAPPER_H_