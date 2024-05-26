#ifndef BUFFEREDMSG_HPP
#define BUFFEREDMSG_HPP

#include <optional>
#include <queue>
#include <string>
#include <memory>

#include "ArduinoJson.h"
#include "MachineID.hpp"

namespace fabomatic
{
  struct BufferedMsg
  {
    const std::string mqtt_message;
    const std::string mqtt_topic;
    BufferedMsg(const std::string &message, const std::string &topic) : mqtt_message(message), mqtt_topic(topic){};
  };

  class Buffer
  {
  public:
    auto pushMessage(const std::string &message, const std::string &topic) -> void;
    auto getMessage() -> const BufferedMsg;
    auto count() const -> size_t;
    auto toJson() const -> JsonDocument;

    static auto fromJson(const std::string &json_text) -> std::optional<Buffer>;

  private:
    std::deque<BufferedMsg> msg_queue;
    static constexpr auto MAGIC_NUMBER = 1;
  };

} // namespace fabomatic

#endif // BUFFEREDMSG_HPP