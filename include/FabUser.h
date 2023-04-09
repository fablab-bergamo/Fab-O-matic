#ifndef _FAB_USER_H_
#define _FAB_USER_H_

#include <cstdint>
#include <string>
#include <array>
#include "Arduino.h"
#include "conf.h"

namespace card
{
  typedef u_int64_t uid_t;
  static constexpr uid_t INVALID = 0;
  inline std::string uid_str(card::uid_t uid)
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
}

struct FabUser
{
  card::uid_t member_uid = card::INVALID;
  std::string holder_name = "";
  bool authenticated = false;

  FabUser() : member_uid(card::INVALID), holder_name(""), authenticated(false) {}
  FabUser(card::uid_t uid, std::string name, bool authenticated) : member_uid(uid), holder_name(name), authenticated(authenticated) {}
  FabUser(const uint8_t uid[conf::whitelist::UID_BYTE_LEN], std::string name, bool authenticated) : holder_name(name), authenticated(authenticated)
  {
    card::uid_t result = 0;
    for (auto i = (conf::whitelist::UID_BYTE_LEN - 1); i >= 0; i--)
    {
      result <<= 8;
      result |= uid[i];
    }
    this->member_uid = result;
  }
  bool operator==(const FabUser &t) const
  {
    return member_uid == t.member_uid;
  }
};

#endif