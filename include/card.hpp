#ifndef CARD_H_
#define CARD_H_

#include "Arduino.h"
#include "conf.hpp"
#include <sstream>
#include <iomanip>

namespace fablabbg::card
{
  using uid_t = u_int64_t;
  static constexpr uid_t INVALID = 0ULL;
  inline std::string uid_str(const card::uid_t uid)
  {
    uint64_t number = static_cast<uint64_t>(uid);
    uint32_t long1 = static_cast<uint32_t>(number & 0xFFFF0000) >> 16;
    uint32_t long2 = static_cast<uint32_t>(number & 0x0000FFFF);

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << std::hex << long1;
    ss << std::setfill('0') << std::setw(4) << std::hex << long2;
    return ss.str();
  }

  constexpr inline uid_t from_array(const uint8_t uid[conf::rfid_tags::UID_BYTE_LEN])
  {
    card::uid_t result = 0;
    for (auto i = (conf::rfid_tags::UID_BYTE_LEN - 1); i >= 0; i--)
    {
      result <<= 8;
      result |= uid[i];
    }
    return result;
  }
  inline void print(uint64_t uid)
  {
    Serial.printf("%s", card::uid_str(uid).c_str());
  }
}
#endif // CARD_H_