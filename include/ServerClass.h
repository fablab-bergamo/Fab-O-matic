#include "MemberClass.h"
#include "WiFi.h"
#include <array>
#ifndef _SERVER_H_  
#define _SERVER_H_

class ServerClass
{
private:
  /* data */
  std::array<card::uid_t, 10> _whitelist;
  const std::string _ssid;
  const std::string _password;
  bool online;
  bool _serverQuery(MemberClass member_card);
  bool _isWhiteListed(MemberClass member_card);
  WiFiClass WiFiConnection;
public:
  ServerClass(const std::array<card::uid_t, 10> whitelist, const std::string ssid, const std::string password);
  ~ServerClass();
  bool isAuthorized(MemberClass& member_card);

  bool isOnline();
  void connect();
  void setOnline(bool online);



};

#endif  // _SERVER_H_