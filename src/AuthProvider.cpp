#include <string>
#include <cstdint>

#include "AuthProvider.h"
#include "FabServer.h"

namespace Board
{
    // Only main.cpp instanciates the variables through Board.h file
    extern FabServer server;
}

FabUser AuthProvider::is_in_cache(card::uid_t uid) const
{
  for (FabUser n : this->cache) {
    if (n.member_uid = uid) {
      return n;
    }
  }
  return FabUser(card::INVALID, "", false, FabUser::UserLevel::UNKNOWN);
}

void AuthProvider::add_in_cache(card::uid_t uid, std::string name, FabUser::UserLevel level) 
{
  // Check if already in cache
  if (this->is_in_cache(uid).authenticated)
    return;
  
  // Add into the list
  FabUser new_elem(uid, name, true, level);
  this->cache.push_front(new_elem);

  // Keep cache size under CACHE_LEN
  if (this->cache.size() > conf::whitelist::CACHE_LEN) {
    this->cache.pop_back();
  }
}

bool AuthProvider::tryLogin(card::uid_t uid, FabUser& out)
{
    FabUser member(uid, "???", false, FabUser::UserLevel::UNKNOWN);
    std::string uid_str = card::uid_str(member.member_uid);
    Serial.printf("tryLogin called for %s\n", uid_str.c_str());
    
    if (!Board::server.isOnline())
        Board::server.connect();
    
    if (Board::server.isOnline()) {
        auto response = Board::server.checkCard(uid);
        if (response.request_ok && response.request_ok) {
          out.authenticated = true;
          out.holder_name = response.holder_name;
          out.member_uid = uid;
          out.user_level = response.user_level;
          // Cache the positive result
          this->add_in_cache(out.member_uid, out.holder_name, response.user_level);
          Serial.println(" -> online check OK");
          return true;
        }
        out.authenticated = false;
        Serial.println(" -> online check NOK");
        return false;
    } 
    else
    {
      auto result = this->WhiteListLookup(uid);
      if (result.found) {
          member.authenticated = true;
          member.holder_name = std::get<2>(result.element);
          member.member_uid = uid;
          member.user_level = std::get<1>(result.element);
          out = member;
          Serial.println(" -> whilelist check OK");
          return true;
      }
    }
    out = member;
    Serial.println(" -> whilelist check NOK");
    return false;
}

AuthProvider::LookupResult AuthProvider::WhiteListLookup(card::uid_t uid) const
{
  for (auto i = 0; i < this->whitelist.size(); i++)
  {
    if (std::get<0>(this->whitelist[i]) == uid)
    {
      return {true, this->whitelist[i]};
    }
  }
  return {false, std::make_tuple<card::uid_t, FabUser::UserLevel, std::string_view>(0, FabUser::UserLevel::UNKNOWN, "")};
}

