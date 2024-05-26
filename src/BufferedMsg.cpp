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
      ESP_LOGI(TAG, "Buffered %s on %s, %lu messages queued", message.c_str(), topic.c_str(), msg_queue.size());
    }
  }

  auto Buffer::push_front(const std::string &message, const std::string &topic) -> void
  {
    if constexpr (conf::debug::ENABLE_BUFFERING)
    {
      BufferedMsg msg{message, topic};
      msg_queue.push_front(msg);
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
    return elem;
  }

  auto Buffer::count() const -> size_t
  {
    return msg_queue.size();
  }

  auto Buffer::toJson() const -> JsonDocument
  {
    JsonDocument doc;
    doc["VERSION"] = MAGIC_NUMBER;
    auto json_elem = doc.createNestedArray("messages");

    for (auto elem = msg_queue.begin(); elem != msg_queue.end(); elem++)
    {
      auto obj = json_elem.createNestedObject();
      obj["topic"] = elem->mqtt_topic;
      obj["message"] = elem->mqtt_message;
    }
    return doc;
  }

  auto Buffer::fromJson(const std::string &json_text) -> std::optional<Buffer>
  {
    Buffer buff;
    JsonDocument doc;

    const auto result = deserializeJson(doc, json_text);
    if (result != DeserializationError::Ok)
    {
      ESP_LOGE(TAG, "Buffer::fromJson() : deserializeJson failed with code %s", result.c_str());
      ESP_LOGE(TAG, "Buffer::fromJson() : %s", json_text.c_str());
      return std::nullopt;
    }

    // Check that the version is the same
    auto version = doc["VERSION"].as<unsigned int>();
    if (version != MAGIC_NUMBER)
    {
      ESP_LOGD(TAG, "Buffer::fromJson() : wrong version number (%d, expected %d)", version, MAGIC_NUMBER);
      return std::nullopt;
    }

    for (const auto &elem : doc["messages"].as<JsonArray>())
    {
      buff.push_back(elem["message"].as<std::string>(),
                     elem["topic"].as<std::string>());
    }

    ESP_LOGD(TAG, "Buffer::fromJson() : data deserialized successfully");

    return buff;
  }

} // namespace fabomatic