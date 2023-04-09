#ifndef _SERVER_H_
#define _SERVER_H_

#include "FabMember.h"
#include "WiFi.h"
#include <array>
#include "conf.h"

class FabServer
{
private:
  const std::string wifi_ssid;
  const std::string wifi_password;
  const std::array<card::uid_t, conf::whitelist::LEN> whitelist;
  bool online;
  bool serverQuery(const FabMember &member_card) const;
  bool isWhiteListed(const FabMember &member_card) const;
  WiFiClass WiFiConnection;

public:
  FabServer(const std::array<card::uid_t, conf::whitelist::LEN> whitelist, const std::string ssid, const std::string password);
  ~FabServer() = default;

  bool isAuthorized(const FabMember &member_card) const;
  bool isOnline() const;
  void connect();
  void setOnline(bool online);

  // Rule of 5 https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five
  FabServer(const FabServer&) = delete; // copy constructor
  FabServer& operator=(const FabServer&) = delete;
  FabServer(FabServer&&) = delete; // move constructor
  FabServer& operator=(FabServer&&) = delete; // move assignment
};

#endif // _SERVER_H_