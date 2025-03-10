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
  /// @brief Message that can be saved in Flash for future replay
  struct BufferedMsg
  {
    std::string mqtt_message{};
    std::string mqtt_topic{};
    bool wait_for_answer = false;
    BufferedMsg() = default;
    BufferedMsg(const std::string &message, const std::string &topic, bool wait) : mqtt_message(message), mqtt_topic(topic), wait_for_answer{wait} {};
    BufferedMsg(const BufferedMsg &source) = default;
    BufferedMsg(BufferedMsg &source) = default;
  };

  /// @brief Class containing the buffered messages for future replay
  class Buffer
  {
  private:
    std::deque<BufferedMsg> msg_queue;
    bool has_changed{true};
    static constexpr auto MAGIC_NUMBER = 1;

  public:
    auto push_back(const BufferedMsg &message) -> void;
    auto push_front(const BufferedMsg &message) -> void;
    auto getMessage() -> const BufferedMsg;
    auto count() const -> size_t;
    auto toJson(JsonDocument &doc, const std::string &element_name) const -> void;
    auto hasChanged() const -> bool { return has_changed; };
    auto setChanged(bool new_value) -> void { has_changed = new_value; };

    static auto fromJsonElement(const JsonObject &json_obj) -> std::optional<Buffer>;
    static constexpr auto MAX_MESSAGES = 40;
  };

  /// @brief Query to be replayed
  class BufferedQuery final : public MQTTInterface::Query
  {
  private:
    std::string_view mqtt_value;
    std::string_view mqtt_topic;
    bool wait_for_answer;

  public:
    BufferedQuery() = delete;
    constexpr BufferedQuery(const std::string_view &value,
                            const std::string_view &topic,
                            bool wait) : mqtt_value(value),
                                         mqtt_topic(topic),
                                         wait_for_answer{wait} {};

    [[nodiscard]] auto payload() const -> const std::string override
    {
      auto value = std::string{mqtt_value};
      if (value.find("}") == value.size() - 1 &&
          value.find("replay") == value.npos)
      {
        value.erase(value.end() - 1);
        value += ", \"replay\":true}";
      }
      return value;
    };

    [[nodiscard]] auto waitForReply() const -> bool override
    {
      return wait_for_answer;
    };

    [[nodiscard]] auto buffered() const -> bool override
    {
      return false;
    };

    [[nodiscard]] auto topic() const -> std::string
    {
      return std::string(mqtt_topic);
    };
  };

} // namespace fabomatic

#endif // BUFFEREDMSG_HPP