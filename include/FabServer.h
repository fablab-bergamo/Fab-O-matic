#ifndef _SERVER_H_
#define _SERVER_H_

#include "FabUser.h"
#include "WiFi.h"
#include <array>
#include "conf.h"

class FabServer
{
private:
  const std::string wifi_ssid;
  const std::string wifi_password;
  bool online;
  FabUser serverQuery(card::uid_t uid) const;
  WiFiClass WiFiConnection;

public:
  FabServer(const std::string ssid, const std::string password);
  ~FabServer() = default;

  FabUser checkCard(card::uid_t uid) const;
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