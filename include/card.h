#ifndef CARD_H_
#define CARD_H_

#include "Arduino.h"

namespace card
{
  using uid_t = u_int64_t;
  static constexpr uid_t INVALID = 0ULL;
  inline std::string uid_str(const card::uid_t uid)
  {
    uint64_t number = static_cast<uint64_t>(uid);
    uint32_t long1 = static_cast<uint32_t>(number & 0xFFFF0000) >> 16;
    uint32_t long2 = static_cast<uint32_t>(number & 0x0000FFFF);

    char buffer[9] = {0};
    snprintf(buffer, 5, "%04X", long1);
    snprintf(&buffer[4], 5, "%04X", long2);

    std::string output(buffer, 9);
    return output;
  }

  inline uid_t from_array(const uint8_t uid[conf::whitelist::UID_BYTE_LEN])
  {
    card::uid_t result = card::INVALID;
    for (auto i = (conf::whitelist::UID_BYTE_LEN - 1); i >= 0; i--)
    {
      result <<= 8;
      result |= uid[i];
    }
    return result;
  }
}
#endif  // CARD_H_