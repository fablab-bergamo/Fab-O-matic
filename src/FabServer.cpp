#include "FabServer.h"

#include <string>
#include <cstdint>

FabServer::FabServer(const std::array<card::uid_t, conf::whitelist::LEN> whitelist, const std::string ssid, const std::string password) : whitelist(whitelist),
                                                                                                                                          wifi_ssid(ssid),
                                                                                                                                          wifi_password(password),
                                                                                                                                          online(false) {}

bool FabServer::isAuthorized(const FabMember &member_card) const
{
  if (this->isOnline())
  {
    return this->serverQuery(member_card);
  }
  else
  {
    if (this->isWhiteListed(member_card))
    {
      return true;
    }
    return false;
  }
}

bool FabServer::isOnline() const
{
  return online;
}

void FabServer::setOnline(bool online)
{
  this->online = online;
}

bool FabServer::isWhiteListed(const FabMember &member_card) const
{
  for (int i = 0; i < this->whitelist.size(); i++)
  {
    if (this->whitelist[i] == member_card.getUid())
    {
      return true;
    }
  }
  return false;
}

bool FabServer::serverQuery(const FabMember &member_card) const
{
  /* TODO */
  if (member_card.getUid() == 0x11223344)
  {
    return true;
  }
  else
    return false;
}

void FabServer::connect()
{
  for (auto i = 0; i < 3; i++)
  {
    this->WiFiConnection.begin(this->wifi_ssid.c_str(), this->wifi_password.c_str());
    if (this->WiFiConnection.status() == WL_CONNECTED)
      break;
    delay(1000);
  }
}