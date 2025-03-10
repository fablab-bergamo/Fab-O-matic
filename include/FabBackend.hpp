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
#include "BufferedMsg.hpp"

namespace fabomatic
{
  /// @brief This class is used to exchange messages with the MQTT broker and the backend
  class FabBackend
  {
  private:
    constexpr static auto MAX_MSG_SIZE = 300;
    enum class PublishResult : uint8_t
    {
      ErrorNotPublished,
      PublishedWithoutAnswer,
      PublishedWithAnswer
    };

    MQTTClient client{MAX_MSG_SIZE}; // Default is 128, and can be reached with some messages
    JsonDocument doc;

    std::string wifi_ssid{""};
    std::string wifi_password{""};
    std::string broker_hostname{""};
    std::string mqtt_user{""};
    std::string mqtt_password{""};
    std::string mqtt_client_name{""};

    MQTTClientCallbackSimpleFunction callback;
    WiFiClient wifi_client;

    std::string topic{""};
    std::string response_topic{""};
    std::string request_topic{""};
    std::string last_query{""};
    std::string last_reply{""};
    std::string last_request{""};

    bool mqtt_connected{false};
    bool answer_pending{false};
    int16_t channel{-1};

    Buffer buffer;
    std::optional<Tasks::time_point> last_unresponsive{std::nullopt};

    auto messageReceived(String &topic, String &payload) -> void;

    template <typename QueryT>
    [[nodiscard]] auto publish(const QueryT &payload) -> PublishResult;

    [[nodiscard]] auto waitForAnswer(std::chrono::milliseconds timeout) -> bool;
    [[nodiscard]] auto publishWithReply(const MQTTInterface::Query &payload) -> PublishResult;

    template <typename RespT, typename QueryT, typename... QueryArgs>
    [[nodiscard]] auto processQuery(QueryArgs &&...) -> std::unique_ptr<RespT>;

    template <typename QueryT, typename... QueryArgs>
    [[nodiscard]] auto processQuery(QueryArgs &&...args) -> bool;

    auto loadBuffer(const Buffer &new_buffer) -> void;

  public:
    FabBackend() = default;

    [[nodiscard]] auto checkCard(const card::uid_t uid) -> std::unique_ptr<MQTTInterface::UserResponse>;
    [[nodiscard]] auto checkMachine() -> std::unique_ptr<MQTTInterface::MachineResponse>;
    [[nodiscard]] auto startUse(const card::uid_t uid) -> std::unique_ptr<MQTTInterface::SimpleResponse>;
    [[nodiscard]] auto inUse(const card::uid_t uid, std::chrono::seconds duration) -> std::unique_ptr<MQTTInterface::SimpleResponse>;
    [[nodiscard]] auto finishUse(const card::uid_t uid, std::chrono::seconds duration) -> std::unique_ptr<MQTTInterface::SimpleResponse>;
    [[nodiscard]] auto registerMaintenance(const card::uid_t maintainer) -> std::unique_ptr<MQTTInterface::SimpleResponse>;
    [[nodiscard]] auto alive() -> bool;
    [[nodiscard]] auto publish(String topic, String payload, bool waitForAnswer) -> bool;
    [[nodiscard]] auto isOnline() const -> bool;
    [[nodiscard]] auto isResponsive() const -> bool;
    [[nodiscard]] auto hasBufferedMsg() const -> bool;
    [[nodiscard]] auto transmitBuffer() -> bool;
    [[nodiscard]] auto saveBuffer() -> bool;
    [[nodiscard]] auto shouldFailFast() const -> bool;

    [[nodiscard]] auto checkBackendRequest() -> std::optional<std::unique_ptr<MQTTInterface::BackendRequest>>;

    auto connect() -> bool;
    auto connectWiFi() -> bool;
    auto loop() -> bool;

    auto configure(const SavedConfig &config) -> void; // Must be called before using the server
    auto disconnect() -> void;
    auto setChannel(int16_t channel) -> void;

    // Rule of 5 https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five
    FabBackend(const FabBackend &) = delete;            // copy constructor
    FabBackend &operator=(const FabBackend &) = delete; // copy assignment
    FabBackend(FabBackend &&) = delete;                 // move constructor
    FabBackend &operator=(FabBackend &&) = delete;      // move assignment
  };
} // namespace fabomatic
#endif // FABBACKEND_HPP_