#ifndef FABBACKEND_HPP_
#define FABBACKEND_HPP_

#include <array>
#include <chrono>
#include <functional>
#include <string>

#include "WiFi.h"
#include <ArduinoJson.h>
#include <MQTTClient.h>

#include "FabUser.hpp"
#include "MQTTtypes.hpp"
#include "SavedConfig.hpp"
#include "conf.hpp"

namespace fablabbg
{
  using namespace ServerMQTT;

  class FabBackend
  {
  private:
    constexpr static auto MAX_MSG_SIZE = 255;

    std::string wifi_ssid;
    std::string wifi_password;
    std::string server_ip;
    std::string mqtt_user;
    std::string mqtt_password;
    std::string mqtt_client_name;

    MQTTClientCallbackSimpleFunction callback;
    WiFiClient wifi_client;
    MQTTClient client{MAX_MSG_SIZE}; // Default is 128, and can be reached with some messages
    StaticJsonDocument<MAX_MSG_SIZE> doc;

    std::string topic{""};
    std::string response_topic{""};
    std::string last_query{""};
    std::string last_reply{""};

    bool online = false;
    bool answer_pending = false;
    int32_t channel{-1};

    void messageReceived(String &topic, String &payload);
    bool publish(const Query &payload);
    bool waitForAnswer(milliseconds timeout);
    bool publishWithReply(const Query &payload);

    template <typename RespT, typename QueryT, typename... QueryArgs>
    [[nodiscard]] std::unique_ptr<RespT> processQuery(QueryArgs &&...);

  public:
    FabBackend() = default;

    [[nodiscard]] std::unique_ptr<UserResponse> checkCard(const card::uid_t uid);
    [[nodiscard]] std::unique_ptr<MachineResponse> checkMachine();
    [[nodiscard]] std::unique_ptr<SimpleResponse> startUse(const card::uid_t uid);
    [[nodiscard]] std::unique_ptr<SimpleResponse> inUse(const card::uid_t uid, std::chrono::seconds duration);
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

    void setChannel(int32_t channel);

    // Rule of 5 https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five
    FabBackend(const FabBackend &) = delete;            // copy constructor
    FabBackend &operator=(const FabBackend &) = delete; // copy assignment
    FabBackend(FabBackend &&) = delete;                 // move constructor
    FabBackend &operator=(FabBackend &&) = delete;      // move assignment
  };
} // namespace fablabbg
#endif // FABBACKEND_HPP_