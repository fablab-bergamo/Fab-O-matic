#include "BufferedMsg.hpp"

#include <inttypes.h>
#include <ArduinoJson.h>
#include "Logging.hpp"

namespace fabomatic
{

  auto Buffer::push_back(const BufferedMsg &message) -> void
  {
    if constexpr (conf::debug::ENABLE_BUFFERING)
    {
      msg_queue.push_back(message);
      if (msg_queue.size() > MAX_MESSAGES)
      {
        msg_queue.pop_front();
        ESP_LOGW(TAG, "Removing first message from the buffer");
      }
      has_changed = true;

      ESP_LOGI(TAG, "Buffered %s on %s, %u messages queued",
               message.mqtt_message.c_str(),
               message.mqtt_topic.c_str(),
               msg_queue.size());
    }
  }

  auto Buffer::push_front(const BufferedMsg &message) -> void
  {
    if constexpr (conf::debug::ENABLE_BUFFERING)
    {
      msg_queue.push_front(message);
      if (msg_queue.size() > MAX_MESSAGES)
      {
        msg_queue.pop_back();
        ESP_LOGW(TAG, "Removing last message from the buffer");
      }
      has_changed = true;

      ESP_LOGI(TAG, "Buffered %s on %s, %u messages queued ",
               message.mqtt_message.c_str(),
               message.mqtt_topic.c_str(),
               msg_queue.size());
    }
  }

  auto Buffer::getMessage() -> const BufferedMsg
  {
    if (msg_queue.size() == 0)
    {
      ESP_LOGE(TAG, "Calling getMessage() on empty queue!");
      return {"", "", false};
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
      obj_msg["tp"] = elem->mqtt_topic;
      obj_msg["msg"] = elem->mqtt_message;
      obj_msg["wait"] = elem->wait_for_answer;
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
      const auto &msg = BufferedMsg{elem["msg"].as<std::string>(),
                                    elem["tp"].as<std::string>(),
                                    elem["wait"].as<bool>()};
      buff.push_back(msg);
    }

    ESP_LOGD(TAG, "Buffer::fromJsonElement() : data loaded successfully");

    return buff;
  }

} // namespace fabomatic