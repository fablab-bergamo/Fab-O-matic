#include "FabServer.h"

#include <string>
#include <cstdint>

FabServer::FabServer(const std::string ssid, const std::string password) : wifi_ssid(ssid), wifi_password(password), online(false) {}

bool FabServer::isOnline() const
{
  return online;
}

bool FabServer::connect()
{
  // Connect WiFi if needed
  if (this->WiFiConnection.status() != WL_CONNECTED) 
  {
    for (auto i = 0; i < 3; i++)
    {
      this->WiFiConnection.begin(this->wifi_ssid.c_str(), this->wifi_password.c_str());
      if (this->WiFiConnection.status() == WL_CONNECTED)
        Serial.println("WiFi connection successfull");
        break;
      delay(1000);
    }
  }

  // Check server
  if (this->WiFiConnection.status() == WL_CONNECTED)
  {
    // TODO - check if the server can be reached
    this->online = true;
  }
  else
  {
    this->online = false;
  }

  return this->online;
}

FabServer::UserResponse FabServer::checkCard(card::uid_t uid) const
{
  UserResponse reply{false, false};
  try
  {
    if (this->isOnline())
    {
      // TODO make a request to a server
      reply.is_valid = true;
      reply.request_ok = true;
      reply.holder_name = "FABMEMBER";
    }
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
  }
  return reply;
}

FabServer::MachineResponse FabServer::checkMachine(Machine::MachineID mid) const
{
  MachineResponse reply{false, false, true, false};
  try
  {
    if (this->isOnline())
    {
      // TODO make a request to a server
      reply.is_valid = true;
      reply.needs_maintenance = false;
      reply.request_ok = true;
      reply.allowed = true;
    }
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
    reply.request_ok = false;
  }
  return reply;
}

FabServer::UseResponse FabServer::startUse(card::uid_t uid, Machine::MachineID mid) const
{
  UseResponse reply{false};
  try
  {
    if (this->isOnline())
    {
      // TODO make a request to a server
      reply.request_ok = true;
    }
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
    reply.request_ok = false;
  }
  return reply;
}

FabServer::UseResponse FabServer::finishUse(card::uid_t uid, Machine::MachineID mid, uint16_t duration_s) const
{
  UseResponse reply{false};
  try
  {
    if (this->isOnline())
    {
      // TODO make a request to a server
      reply.request_ok = true;
    }
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
    reply.request_ok = false;
  }
  return reply;
}