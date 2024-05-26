#include "BufferedMsg.hpp"

#include <ArduinoJson.h>
#include "Logging.hpp"

namespace fabomatic
{

  auto Buffer::push_back(const std::string &message, const std::string &topic) -> void
  {
    if constexpr (conf::debug::ENABLE_BUFFERING)
    {
      BufferedMsg msg{message, topic};
      msg_queue.push_back(msg);
      has_changed = true;

      ESP_LOGI(TAG, "Buffered %s on %s, %lu messages queued", message.c_str(), topic.c_str(), msg_queue.size());
    }
  }

  auto Buffer::push_front(const std::string &message, const std::string &topic) -> void
  {
    if constexpr (conf::debug::ENABLE_BUFFERING)
    {
      BufferedMsg msg{message, topic};
      msg_queue.push_front(msg);
      has_changed = true;

      ESP_LOGI(TAG, "Buffered %s on %s, %lu messages queued", message.c_str(), topic.c_str(), msg_queue.size());
    }
  }

  auto Buffer::getMessage() -> const BufferedMsg
  {
    if (msg_queue.size() == 0)
    {
      ESP_LOGE(TAG, "Calling getMessage() on empty queue!");
      return {"", ""};
    }

    auto elem = msg_queue.front();
    msg_queue.pop_front();
    has_changed = true;

    return elem;
  }

  auto Buffer::count() const -> size_t
  {
    return msg_queue.size();
  }

  auto Buffer::toJson(JsonDocument &doc, const std::string &element_name) const -> void
  {
    auto obj = doc.createNestedObject(element_name);
    obj["VERSION"] = MAGIC_NUMBER;
    auto json_elem = obj.createNestedArray("messages");

    for (auto elem = msg_queue.begin(); elem != msg_queue.end(); elem++)
    {
      auto obj_msg = json_elem.createNestedObject();
      obj_msg["topic"] = elem->mqtt_topic;
      obj_msg["message"] = elem->mqtt_message;
    }
  }

  auto Buffer::fromJsonElement(const JsonObject &json_obj) -> std::optional<Buffer>
  {
    Buffer buff;

    // Check that the version is the same
    auto version = json_obj["VERSION"].as<unsigned int>();
    if (version != MAGIC_NUMBER)
    {
      ESP_LOGD(TAG, "Buffer::fromJson() : wrong version number (%d, expected %d)", version, MAGIC_NUMBER);
      return std::nullopt;
    }

    for (const auto &elem : json_obj["messages"].as<JsonArray>())
    {
      buff.push_back(elem["message"].as<std::string>(),
                     elem["topic"].as<std::string>());
    }

    ESP_LOGD(TAG, "Buffer::fromJsonElement() : data loaded successfully");

    return buff;
  }

} // namespace fabomatic