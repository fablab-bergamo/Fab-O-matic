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
  bool _serverQuery(FabMember member_card);
  bool _isWhiteListed(FabMember member_card);
  WiFiClass WiFiConnection;

public:
  FabServer(const std::array<card::uid_t, conf::whitelist::LEN> whitelist, const std::string ssid, const std::string password);
  ~FabServer() = default;

  bool isAuthorized(FabMember &member_card);
  bool isOnline();
  void connect();
  void setOnline(bool online);
};

#endif // _SERVER_H_