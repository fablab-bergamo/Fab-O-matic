#ifndef FABSERVER_H_
#define FABSERVER_H_

#include <array>
#include <chrono>
#include <functional>
#include <string>

#include <ArduinoJson.h>
#include "WiFi.h"
#include <MQTTClient.h>

#include "FabUser.hpp"
#include "conf.hpp"
#include "MQTTtypes.hpp"
#include "SavedConfig.hpp"

namespace fablabbg
{
  using namespace ServerMQTT;

  class FabServer
  {
  private:
    std::string wifi_ssid;
    std::string wifi_password;
    std::string server_ip;
    std::string mqtt_user;
    std::string mqtt_password;
    std::string mqtt_client_name;

    MQTTClientCallbackSimpleFunction callback;
    WiFiClass WiFiConnection;
    WiFiClient net;
    MQTTClient client;
    StaticJsonDocument<256> doc;

    std::string topic{""};
    std::string response_topic{""};
    std::string last_query{""};
    std::string last_reply{""};

    bool online = false;
    bool answer_pending = false;
    uint8_t channel = -1;

    void messageReceived(String &topic, String &payload);
    bool publish(const Query &payload);
    bool waitForAnswer();
    bool publishWithReply(const Query &payload);

    template <typename RespT, typename QueryT, typename... QueryArgs>
    [[nodiscard]] std::unique_ptr<RespT> processQuery(QueryArgs &&...);

    static constexpr unsigned int MAX_MQTT_LENGTH = 128;

  public:
    FabServer() = default;

    [[nodiscard]] std::unique_ptr<UserResponse> checkCard(const card::uid_t uid);
    [[nodiscard]] std::unique_ptr<MachineResponse> checkMachine();
    [[nodiscard]] std::unique_ptr<SimpleResponse> startUse(const card::uid_t uid);
    [[nodiscard]] std::unique_ptr<SimpleResponse> finishUse(const card::uid_t uid, std::chrono::seconds duration);
    [[nodiscard]] std::unique_ptr<SimpleResponse> registerMaintenance(const card::uid_t maintainer);
    [[nodiscard]] std::unique_ptr<SimpleResponse> alive();
    [[nodiscard]] bool publish(String topic, String payload);

    bool isOnline() const;
    bool connect();
    bool connectWiFi() noexcept;
    bool loop();
    void configure(const SavedConfig &config); // Must be called before using the server
    void disconnect();

    // Rule of 5 https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five
    FabServer(const FabServer &) = delete;            // copy constructor
    FabServer &operator=(const FabServer &) = delete; // copy assignment
    FabServer(FabServer &&) = delete;                 // move constructor
    FabServer &operator=(FabServer &&) = delete;      // move assignment
  };
} // namespace fablabbg
#endif // FABSERVER_H_