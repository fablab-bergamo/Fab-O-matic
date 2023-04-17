#ifndef FABUSER_H_
#define FABUSER_H_

#include <cstdint>
#include <string>
#include <array>
#include "Arduino.h"
#include "conf.h"
#include "card.h"
#include <sstream>

struct FabUser
{
  enum class UserLevel
  {
    UNKNOWN,
    FABLAB_USER,
    FABLAB_ADMIN
  };

  card::uid_t card_uid = card::INVALID;
  std::string holder_name = "";
  bool authenticated = false;
  UserLevel user_level = UserLevel::UNKNOWN;

  FabUser() = default;

  FabUser(const card::uid_t uid, std::string_view name, bool authenticated, UserLevel level) : 
    card_uid(uid), holder_name(name), authenticated(authenticated), user_level(level) {}

  FabUser(const uint8_t uid[conf::whitelist::UID_BYTE_LEN], std::string_view name, bool authenticated, UserLevel level) :
    holder_name(name), authenticated(authenticated), user_level(level)
  {
    this->card_uid = card::from_array(uid);
  }
  bool operator==(const FabUser &t) const
  {
    return card_uid == t.card_uid;
  }
  std::string toString() const
  {
    std::stringstream sstream;
    
    sstream << "User (Auth:" << authenticated;
    sstream << ", UID: " << std::hex << card_uid;
    sstream << ", Name:" << holder_name;

    auto i_level = static_cast<typename std::underlying_type<UserLevel>::type>(user_level);
    sstream << ", level:" << i_level << ")";

    return sstream.str();
  }
};

#endif  // FABUSER_H_