#ifndef _RFID_WRAPPER_H_
#define _RFID_WRAPPER_H_

#include "conf.h"
#include <string>
#include "card.h"

class RFIDWrapper
{
public:
    RFIDWrapper();
    bool init();
    bool IsNewCardPresent();
    bool ReadCardSerial();
    card::uid_t GetUid() const;
};

#endif