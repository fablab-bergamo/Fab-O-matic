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
    bool IsNewCardPresent() const;
    bool ReadCardSerial() const;
    card::uid_t GetUid() const;
};

#endif