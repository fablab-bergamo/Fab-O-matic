#ifndef FABSERVER_H_
#define FABSERVER_H_

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
  const std::string server_ip;
  bool online;
  WiFiClass WiFiConnection;

public:
  struct UserResponse
  {
    bool request_ok;               /* True if the request was processed by the server*/
    bool is_valid;                 /* True if the user is valid */
    std::string_view holder_name;  /* Name of the user from server DB */
    FabUser::UserLevel user_level; /* User priviledges */
  };
  struct MachineResponse
  {
    bool request_ok;        /* True if the request was processed by the server */
    bool is_valid;          /* True if the machine has a valid ID */
    bool needs_maintenance; /* True if the machine needs maintenance */
    bool allowed;           /* True if the machine can be used by anybody */
  };
  struct SimpleResponse
  {
    bool request_ok; /* True if the request was processed by the server */
  };

  FabServer() = delete;
  FabServer(const std::string_view ssid, const std::string_view password, const std::string_view server_ip);
  ~FabServer() = default;

  UserResponse checkCard(const card::uid_t uid) const;
  MachineResponse checkMachine(const Machine::MachineID mid) const;
  SimpleResponse startUse(const card::uid_t uid, const Machine::MachineID mid) const;
  SimpleResponse finishUse(const card::uid_t uid, const Machine::MachineID mid, uint16_t duration_s) const;
  SimpleResponse registerMaintenance(const card::uid_t maintainer, const Machine::MachineID mid) const;
  SimpleResponse alive(const Machine::MachineID mid);

  bool isOnline() const;
  bool connect();

  // Rule of 5 https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five
  FabServer(const FabServer &) = delete; // copy constructor
  FabServer &operator=(const FabServer &) = delete; // copy assignment
  FabServer(FabServer &&) = delete;            // move constructor
  FabServer &operator=(FabServer &&) = delete; // move assignment
};

#endif // FABSERVER_H_