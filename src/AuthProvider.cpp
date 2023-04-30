#include "AuthProvider.h"

#include <string>
#include <cstdint>
#include "FabServer.h"

namespace Board
{
  // Only main.cpp instanciates the variables through Board.h file
  extern FabServer server;
} // namespace Board

AuthProvider::AuthProvider(WhiteList whitelist) : whitelist(whitelist) {}

/// @brief Checks if the cache contains the card ID, and uses that if available
/// @param uid card id
/// @return FabUser if found
std::optional<FabUser> AuthProvider::is_in_cache(card::uid_t uid) const
{
  auto elem = std::find_if(this->cache.begin(), this->cache.end(),
                           [uid](const auto &input)
                           { return input.card_uid == uid; });

  if (elem == end(this->cache))
  {
    return std::nullopt;
  }

  return {*elem};
}

/// @brief Cache the user request
/// @param uid card id of the user
/// @param name name of the user to be cached
/// @param level priviledge level of the user
void AuthProvider::add_in_cache(card::uid_t uid, std::string name, FabUser::UserLevel level) const
{
  // Check if already in cache
  if (this->is_in_cache(uid).has_value())
    return;

  // Add into the list
  FabUser new_elem(uid, name, true, level);
  this->cache.push_front(new_elem);

  // Keep cache size under CACHE_LEN
  if (this->cache.size() > conf::whitelist::CACHE_LEN)
  {
    this->cache.pop_back();
  }
}

/// @brief Verifies the card ID against the server (if available) or the whitelist
/// @param uid card ID
/// @param out a FabUser with an authenticated flag==true if server or whitelist confirmed the ID
/// @return false if the user was not found on server or whitelist
std::optional<FabUser> AuthProvider::tryLogin(card::uid_t uid) const
{
  FabUser user;
  std::string uid_str = card::uid_str(uid);

  if (conf::debug::ENABLE_LOGS)
    Serial.printf("tryLogin called for %s\r\n", uid_str.c_str());

  if (!Board::server.isOnline())
    Board::server.connect();

  if (Board::server.isOnline())
  {
    if (auto response = Board::server.checkCard(uid); response->request_ok && response->is_valid)
    {
      user.authenticated = true;
      user.card_uid = uid;
      user.holder_name = response->holder_name;
      user.user_level = response->user_level;
      // Cache the positive result
      this->add_in_cache(uid, user.holder_name, response->user_level);

      if (conf::debug::ENABLE_LOGS)
        Serial.printf(" -> online check OK (%s)\r\n", user.toString().c_str());

      return user;
    }
    else
    {
      if (conf::debug::ENABLE_LOGS)
        Serial.println(" -> online check NOK");

      user.authenticated = false;

      if (response->request_ok)
        return std::nullopt;

      // If request failed, we need to check the whitelist
    }
  }

  if (auto result = this->WhiteListLookup(uid); result.has_value())
  {
    auto [card, level, name] = result.value();
    user.card_uid = card;
    user.authenticated = true;
    user.user_level = level;
    user.holder_name = name;
    if (conf::debug::ENABLE_LOGS)
      Serial.printf(" -> whilelist check OK (%s)\r\n", user.toString().c_str());
    return user;
  }

  if (conf::debug::ENABLE_LOGS)
    Serial.println(" -> whilelist check NOK");
  return std::nullopt;
}

/// @brief Checks if the card ID is whitelisted
/// @param uid card ID
/// @return a whitelistentry object if the card is found in whitelist
std::optional<WhiteListEntry> AuthProvider::WhiteListLookup(card::uid_t candidate_uid) const
{
  auto elem = std::find_if(this->whitelist.begin(), this->whitelist.end(),
                           [candidate_uid](const auto &input)
                           {
                             auto [w_uid, w_level, w_name] = input;
                             return w_uid == candidate_uid;
                           });

  if (elem == end(this->whitelist))
  {
    Serial.printf("%s not found in whitelist\r\n", card::uid_str(candidate_uid).c_str());
    return std::nullopt;
  }

  return {*elem};
}
