#include "FabServer.h"
#include <string>
#include <cstdint>

FabServer::FabServer(const std::array<card::uid_t, 10> whitelist, const std::string ssid, const std::string password)
    : _whitelist(whitelist), _ssid(ssid), _password(password)
{
  online = false;
}

bool FabServer::isAuthorized(FabMember &member_card)
{
  if (this->isOnline())
  {
    return this->_serverQuery(member_card);
  }
  else
  {
    bool answer = this->_isWhiteListed(member_card);
    member_card.setName("MEMBER");
    return this->_isWhiteListed(member_card);
  }
}

bool FabServer::isOnline()
{
  return online;
}

void FabServer::setOnline(bool online)
{
  this->online = online;
}

bool FabServer::_isWhiteListed(FabMember member_card)
{
  for (int i = 0; i < this->_whitelist.size(); i++)
  {
    if (this->_whitelist[i] == member_card.getUid())
    {
      return true;
    }
  }
  return false;
}

bool FabServer::_serverQuery(FabMember member_card)
{
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
    this->WiFiConnection.begin(this->_ssid.c_str(), this->_password.c_str());
    if (this->WiFiConnection.status() == WL_CONNECTED)
      break;
    delay(1000);
  }
}