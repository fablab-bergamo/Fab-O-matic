#include <string>
#include <cstdint>

#include "AuthProvider.h"
#include "FabServer.h"

namespace Board
{
  // Only main.cpp instanciates the variables through Board.h file
  extern FabServer server;
}

std::optional<FabUser> AuthProvider::is_in_cache(card::uid_t uid) const
{
  auto elem = std::find_if(this->cache.begin(), this->cache.end(), 
                          [uid](const auto& input){return input.card_uid == uid;});
  
  if (elem == end(this->cache)) {
    return {};
  }
  
  return {*elem};
}

void AuthProvider::add_in_cache(card::uid_t uid, std::string name, FabUser::UserLevel level)
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

bool AuthProvider::tryLogin(card::uid_t uid, FabUser &out)
{
  FabUser member(uid, "???", false, FabUser::UserLevel::UNKNOWN);
  std::string uid_str = card::uid_str(member.card_uid);
  Serial.printf("tryLogin called for %s\n", uid_str.c_str());

  if (!Board::server.isOnline())
    Board::server.connect();

  if (Board::server.isOnline())
  {
    auto response = Board::server.checkCard(uid);
    if (response.request_ok && response.request_ok)
    {
      out.authenticated = true;
      out.holder_name = response.holder_name;
      out.card_uid = uid;
      out.user_level = response.user_level;
      // Cache the positive result
      this->add_in_cache(out.card_uid, out.holder_name, response.user_level);
      Serial.println(" -> online check OK");
      return true;
    }
    out.authenticated = false;
    Serial.println(" -> online check NOK");
    return false;
  }
  else
  {
    if (auto result = this->WhiteListLookup(uid))
    {
      member.authenticated = true;
      member.card_uid = std::get<0>(result.value());
      member.user_level = std::get<1>(result.value());
      member.holder_name = std::get<2>(result.value());
      out = member;
      Serial.println(" -> whilelist check OK");
      return true;
    }
  }
  out = member;
  Serial.println(" -> whilelist check NOK");
  return false;
}

std::optional<WhiteListEntry> AuthProvider::WhiteListLookup(card::uid_t uid) const
{
  auto elem = std::find_if(this->whitelist.begin(), this->whitelist.end(), 
                          [uid](const auto& input){return std::get<0>(input) == uid;});
  
  if (elem == end(this->whitelist)) {
    return {};
  }
  
  return {*elem};
}
