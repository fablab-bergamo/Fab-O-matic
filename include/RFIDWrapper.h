#ifndef _RFID_WRAPPER_H_
#define _RFID_WRAPPER_H_

#include "conf.h"
#include <string>
#include "FabUser.h"

class RFIDWrapper
{
public:
    RFIDWrapper();
    void init();
    bool IsNewCardPresent();
    bool ReadCardSerial();
    FabUser GetUser() const;
    std::string dumpUid() const;
};

#endif