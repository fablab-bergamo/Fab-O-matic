#ifndef FABUSER_H_
#define FABUSER_H_

#include <cstdint>
#include <string>
#include <array>
#include "Arduino.h"
#include "conf.h"
#include "card.h"

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
  FabUser(const card::uid_t uid, const std::string_view name, bool authenticated, UserLevel level) : 
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
  std::string to_string() const
  {
    auto value = static_cast<typename std::underlying_type<UserLevel>::type>(user_level);
    return "User(Auth:" + std::to_string(authenticated) + ", UID:" + std::to_string(card_uid) + ", Name:" + holder_name + ", level:" + std::to_string(value) + ")";
  }
};

#endif  // FABUSER_H_