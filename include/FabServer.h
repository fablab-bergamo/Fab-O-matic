#ifndef _SERVER_H_
#define _SERVER_H_

#include "FabUser.h"
#include "WiFi.h"
#include <array>
#include "conf.h"
#include "Machine.h"
#include <string>

class FabServer
{
private:
  const std::string wifi_ssid;
  const std::string wifi_password;
  bool online;
  WiFiClass WiFiConnection;

public:
  struct UserResponse {
    bool request_ok; /* True if the request was processed by the server*/
    bool is_valid; /* True if the user is valid */
    std::string holder_name; /* Name of the user from server DB */
    FabUser::UserLevel user_level;
  };
  struct MachineResponse {
    bool request_ok; /* True if the request was processed by the server */
    bool is_valid; /* True if the machine has a valid ID */
    bool needs_maintenance; /* True if the machine needs maintenance */
    bool allowed; /* True if the machine can be used by anybody */
  };
  struct UseResponse {
    bool request_ok; /* True if the request was processed by the server */
  };

  FabServer(const std::string ssid, const std::string password);
  ~FabServer() = default;

  UserResponse checkCard(card::uid_t uid) const;
  MachineResponse checkMachine(Machine::MachineID mid) const;
  UseResponse startUse(card::uid_t uid, Machine::MachineID mid) const;
  UseResponse finishUse(card::uid_t uid, Machine::MachineID mid, uint16_t duration_s) const;

  bool isOnline() const;
  bool connect();

  // Rule of 5 https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five
  FabServer(const FabServer&) = delete; // copy constructor
  FabServer& operator=(const FabServer&) = delete;
  FabServer(FabServer&&) = delete; // move constructor
  FabServer& operator=(FabServer&&) = delete; // move assignment
};

#endif // _SERVER_H_