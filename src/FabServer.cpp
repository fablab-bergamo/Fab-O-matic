#include "FabServer.h"

#include <string>
#include <cstdint>

FabServer::FabServer(const std::string ssid, const std::string password) : wifi_ssid(ssid), wifi_password(password), online(false) {}

FabUser FabServer::checkCard(card::uid_t uid) const
{
  FabUser out(uid, "???", false);
  if (this->isOnline())
  {
    return this->serverQuery(uid);
  }
  return out;
}

bool FabServer::isOnline() const
{
  return online;
}

void FabServer::setOnline(bool online)
{
  this->online = online;
}


FabUser FabServer::serverQuery(card::uid_t uid) const
{
   // TODO
   FabUser output(uid, "NOME", true);
   return output;
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