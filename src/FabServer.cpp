#include "FabServer.h"

#include <string>
#include <string_view>
#include <cstdint>

/// @brief FabServer API interface class
/// @param ssid wifi network
/// @param password wifi password
/// @param server_ip server IP address
FabServer::FabServer(const std::string_view ssid, const std::string_view password, const std::string_view server_ip) : wifi_ssid(ssid), wifi_password(password), server_ip(server_ip), online(false) {}

bool FabServer::isOnline() const
{
  return online;
}

/// @brief Establish WiFi connection and connects to FabServer
/// @return true if both operations succeeded
bool FabServer::connect()
{
  constexpr uint8_t NB_TRIES = 3;
  constexpr uint16_t DELAY_MS = 1000;

  // Connect WiFi if needed
  if (this->WiFiConnection.status() != WL_CONNECTED)
  {
    this->WiFiConnection.begin(this->wifi_ssid.c_str(), this->wifi_password.c_str());
    for (auto i = 0; i < NB_TRIES; i++)
    {
      if (conf::debug::DEBUG && this->WiFiConnection.status() == WL_CONNECTED)
        Serial.println("WiFi connection successfull");
      break;
      delay(DELAY_MS);
    }
  }

  // Check server
  if (this->WiFiConnection.status() == WL_CONNECTED)
  {
    // TODO - check if the server can be reached
    if (conf::debug::DEBUG)
    {
      Serial.print("Board IP Address:");
      Serial.println(WiFi.localIP());
    }
    this->online = false;
  }
  else
  {
    this->online = false;
  }

  return this->online;
}

/// @brief Checks if the card ID is known to the server
/// @param uid card uid
/// @return server response (if request_ok)
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
      reply.user_level = FabUser::UserLevel::FABLAB_ADMIN;
    }
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
    reply.request_ok = false;
  }
  return reply;
}

/// @brief Checks the machine status on the server
/// @param mid machine id
/// @return server response (if request_ok)
FabServer::MachineResponse FabServer::checkMachine(Machine::MachineID mid) const
{
  MachineResponse reply{false, false, true, false};
  try
  {
    if (this->isOnline())
    {
      // TODO make a request to a server
      reply.is_valid = true;
      reply.needs_maintenance = true;
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

/// @brief register the starting of a machine usage
/// @param uid card uid
/// @param mid machine id
/// @return server response (if request_ok)
FabServer::SimpleResponse FabServer::startUse(card::uid_t uid, Machine::MachineID mid) const
{
  SimpleResponse reply{false};
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

/// @brief Register end of machine usage
/// @param uid card ID of the machine user
/// @param mid machine used ID
/// @param duration_s duration of usage in seconds
/// @return server response (if request_ok)
FabServer::SimpleResponse FabServer::finishUse(card::uid_t uid, Machine::MachineID mid, uint16_t duration_s) const
{
  SimpleResponse reply{false};
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

FabServer::SimpleResponse FabServer::registerMaintenance(card::uid_t maintainer, Machine::MachineID mid) const
{
  SimpleResponse reply{false};
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