#ifndef BUFFEREDMSG_HPP
#define BUFFEREDMSG_HPP

#include <optional>
#include <queue>
#include <string>
#include <memory>

#include "ArduinoJson.h"
#include "MachineID.hpp"
#include "MQTTtypes.hpp"

namespace fabomatic
{
  struct BufferedMsg
  {
    std::string mqtt_message;
    std::string mqtt_topic;
    BufferedMsg() = default;
    BufferedMsg(const std::string &message, const std::string &topic) : mqtt_message(message), mqtt_topic(topic){};
    BufferedMsg(const BufferedMsg &source) = default;
    BufferedMsg(BufferedMsg &source) = default;
  };

  class Buffer
  {
  public:
    auto push_back(const std::string &message, const std::string &topic) -> void;
    auto push_front(const std::string &message, const std::string &topic) -> void;
    auto getMessage() -> const BufferedMsg;
    auto count() const -> size_t;
    auto toJson() const -> JsonDocument;

    static auto fromJson(const std::string &json_text) -> std::optional<Buffer>;

  private:
    std::deque<BufferedMsg> msg_queue;
    static constexpr auto MAGIC_NUMBER = 1;
  };

  class BufferedQuery final : public ServerMQTT::Query
  {
  private:
    std::string_view mqtt_value;
    std::string_view mqtt_topic;

  public:
    BufferedQuery() = delete;
    constexpr BufferedQuery(const std::string_view &value, const std::string_view &topic) : mqtt_value(value), mqtt_topic(topic){};

    [[nodiscard]] auto payload() const -> const std::string override { return std::string(mqtt_value); };
    [[nodiscard]] auto waitForReply() const -> bool override { return false; };
    [[nodiscard]] auto buffered() const -> bool override { return false; };
    [[nodiscard]] auto topic() const -> std::string { return std::string(mqtt_topic); };
  };

} // namespace fabomatic

#endif // BUFFEREDMSG_HPP