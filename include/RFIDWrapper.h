#ifndef RFIDWRAPPER_H_
#define RFIDWRAPPER_H_

#include <string>
#include <memory>

#include "conf.h"
#include "card.h"
#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"

class RFIDWrapper
{
private:
    std::unique_ptr<MFRC522> mfrc522;
    std::unique_ptr<MFRC522DriverPinSimple> rfid_simple_driver;
    std::unique_ptr<MFRC522DriverSPI> spi_rfid_driver;

public:
    RFIDWrapper();

    bool init() const;
    bool isNewCardPresent() const;
    bool cardStillThere(const card::uid_t original) const;
    bool readCardSerial() const;
    bool selfTest() const;
    void reset() const;
    card::uid_t getUid() const;

    RFIDWrapper(const RFIDWrapper &) = delete;             // copy constructor
    RFIDWrapper &operator=(const RFIDWrapper &x) = delete; // copy assignment
    RFIDWrapper(RFIDWrapper &&) = delete;                  // move constructor
    RFIDWrapper &operator=(RFIDWrapper &&) = delete;       // move assignment
};

#endif // RFIDWRAPPER_H_