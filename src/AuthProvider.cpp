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
  return FabUser(card::INVALID, "", false);
}

void AuthProvider::add_in_cache(card::uid_t uid, std::string name) 
{
  // Check if already in cache
  if (this->is_in_cache(uid).authenticated)
    return;
  
  // Add into the list
  FabUser new_elem(uid, name, true);
  this->cache.push_front(new_elem);

  // Keep cache size under CACHE_LEN
  if (this->cache.size() > conf::whitelist::CACHE_LEN) {
    this->cache.pop_back();
  }
}

bool AuthProvider::tryLogin(card::uid_t uid, FabUser& out)
{
    FabUser member(uid, "???", false);
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
          // Cache the positive result
          this->add_in_cache(out.member_uid, out.holder_name);
          Serial.println(" -> online check OK");
          return true;
        }
        out.authenticated = false;
        Serial.println(" -> online check NOK");
        return false;
    } 
    else
    {
      if (this->isWhiteListed(uid)) {
          member.authenticated = true;
          member.holder_name = "FABLAB";
          member.member_uid = uid;
          out = member;
          Serial.println(" -> whilelist check OK");
          return true;
      }
    }
    out = member;
    Serial.println(" -> whilelist check NOK");
    return false;
}

bool AuthProvider::isWhiteListed(card::uid_t uid) const
{
  for (auto i = 0; i < this->whitelist.size(); i++)
  {
    if (this->whitelist[i] == uid)
    {
      return true;
    }
  }
  return false;
}

