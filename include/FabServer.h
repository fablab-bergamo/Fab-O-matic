#ifndef FABSERVER_H_
#define FABSERVER_H_

#include "FabUser.h"
#include "WiFi.h"
#include <array>
#include "conf.h"
#include "Machine.h"
#include <string>
#include <MQTTClient.h>
#include <functional>
#include <ArduinoJson.h>
#include "MQTTtypes.h"

using namespace ServerMQTT;

class FabServer
{
private:
  const std::string_view wifi_ssid;
  const std::string_view wifi_password;
  const std::string_view server_ip;
  std::string topic = "";
  std::string response_topic = "";

  MQTTClientCallbackSimpleFunction callback;
  WiFiClass WiFiConnection;
  WiFiClient net;
  MQTTClient client;
  StaticJsonDocument<256> doc;
  std::string last_query = "";
  std::string last_reply = "";
  bool online = false;
  bool answer_pending = false;

  void messageReceived(String &topic, String &payload);
  bool publish(const Query &payload);
  bool waitForAnswer();
  bool publishWithReply(const Query &payload);
  String fakeReply() const;

  template <typename RespT, typename QueryT, typename... QueryArgs>
  std::unique_ptr<RespT> processQuery(QueryArgs &&...);

  static constexpr unsigned int MAX_MQTT_LENGTH = 128;

public:
  FabServer() = delete;
  FabServer(std::string_view ssid, std::string_view password, std::string_view server_ip);
  ~FabServer() = default;

  std::unique_ptr<UserResponse> checkCard(const card::uid_t uid);
  std::unique_ptr<MachineResponse> checkMachine(const Machine::MachineID mid);
  std::unique_ptr<SimpleResponse> startUse(const card::uid_t uid, const Machine::MachineID mid);
  std::unique_ptr<SimpleResponse> finishUse(const card::uid_t uid, const Machine::MachineID mid, uint16_t duration_s);
  std::unique_ptr<SimpleResponse> registerMaintenance(const card::uid_t maintainer, const Machine::MachineID mid);
  std::unique_ptr<SimpleResponse> alive(const Machine::MachineID mid);

  bool isOnline() const;
  bool connect();
  bool loop();

  // Rule of 5 https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five
  FabServer(const FabServer &) = delete;            // copy constructor
  FabServer &operator=(const FabServer &) = delete; // copy assignment
  FabServer(FabServer &&) = delete;                 // move constructor
  FabServer &operator=(FabServer &&) = delete;      // move assignment
};

#endif // FABSERVER_H_