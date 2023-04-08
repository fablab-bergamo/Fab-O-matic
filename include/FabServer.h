#ifndef _SERVER_H_
#define _SERVER_H_

#include "FabMember.h"
#include "WiFi.h"
#include <array>
#include "conf.h"

class FabServer
{
private:
  std::array<card::uid_t, conf::whitelist::LEN> _whitelist;
  const std::string _ssid;
  const std::string _password;
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
  FabServer(FabServer const&) = default;
  // copy constructor
  //FabServer(const FabServer &) = default;
  // move constructor
  FabServer(FabServer &&) = default;
  // move assignment
  FabServer &operator=(FabServer &&) = default;
};

#endif // _SERVER_H_