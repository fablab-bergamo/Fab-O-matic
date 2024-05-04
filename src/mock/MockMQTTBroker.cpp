#include "mock/MockMQTTBroker.hpp"
#include "Logging.hpp"
#include "conf.hpp"
#include "secrets.hpp"

#include <ArduinoJson.h>

static const char *const TAG2 = "MockMQTTBroker";
namespace fabomatic
{
  auto MockMQTTBroker::start() -> void
  {
    while (WiFi.status() != WL_CONNECTED)
    { // Wait for the Wi-Fi to connect
      ESP_LOGD(TAG2, "MQTT BROKER: WiFi status changed to %d", WiFi.status());
      is_running = false;
      return;
    }
    if (!is_running)
    {
      is_running = init(conf::mqtt::PORT_NUMBER, true);
      ESP_LOGI(TAG2, "MQTT BROKER: started with result %d", is_running);
    }
  }

  auto MockMQTTBroker::onEvent(sMQTTEvent *event) -> bool
  {
    switch (event->Type())
    {
    case NewClient_sMQTTEventType:
    {
      auto *e = static_cast<sMQTTNewClientEvent *>(event); // NOLINT(unused-variable)
      ESP_LOGD(TAG2, "MQTT BROKER: client connected, id:%s", e->Client()->getClientId().c_str());
    }
    break;
    case Public_sMQTTEventType:
    {
      std::lock_guard<std::mutex> lock(mutex);
      auto *e = static_cast<sMQTTPublicClientEvent *>(event);
      topic = e->Topic();
      payload = e->Payload();

      ESP_LOGI(TAG2, "MQTT BROKER: Received  %s -> %s", topic.c_str(), payload.c_str());
      queries.push({topic, payload, topic + "/reply"});
    }
    break;
    case RemoveClient_sMQTTEventType:
    {
      auto *e = static_cast<sMQTTRemoveClientEvent *>(event); // NOLINT(unused-variable)
      ESP_LOGD(TAG2, "MQTT BROKER: removed client id: %s", e->Client()->getClientId().c_str());
    }
    break;
    case LostConnect_sMQTTEventType:
    {
      is_running = false;
      ESP_LOGD(TAG2, "MQTT BROKER: lost connection");
    }
    break;
    case Subscribe_sMQTTEventType:
    {
      auto *e = static_cast<sMQTTSubUnSubClientEvent *>(event); // NOLINT(unused-variable)
      ESP_LOGD(TAG2, "MQTT BROKER: client %s subscribed to %s", e->Client()->getClientId().c_str(), e->Topic().c_str());
    }
    break;
    case UnSubscribe_sMQTTEventType:
    {
      auto *e = static_cast<sMQTTSubUnSubClientEvent *>(event); // NOLINT(unused-variable)
      ESP_LOGD(TAG2, "MQTT BROKER: got unsubscribe from %s", e->Topic().c_str());
    }
    break;
    default:
      ESP_LOGD(TAG2, "MQTT BROKER: unhandled event %d", event->Type());
      break;
    }
    return true;
  }

  auto MockMQTTBroker::isRunning() const -> bool
  {
    return is_running;
  }

  /// @brief Returns a fake server reply for testing purposes
  /// @return json payload
  auto MockMQTTBroker::defaultReplies(const std::string &query) const -> const std::string
  {
    if (query.find("checkmachine") != std::string::npos)
    {
      return "{\"request_ok\":true,\"is_valid\":true,\"allowed\":true,\"maintenance\":false,\"logoff\":30,\"name\":\"ENDER_1\",\"type\":1,\"description\":\"\"}";
    }

    if (query.find("maintenance") != std::string::npos)
    {
      return "{\"request_ok\":true}";
    }

    if (query.find("startuse") != std::string::npos)
    {
      return "{\"request_ok\":true}";
    }

    if (query.find("inuse") != std::string::npos)
    {
      return "{\"request_ok\":true}";
    }

    if (query.find("stopuse") != std::string::npos)
    {
      return "{\"request_ok\":true}";
    }

    if (query.find("checkuser") != std::string::npos)
    {
      JsonDocument doc;
      if (deserializeJson(doc, query) == DeserializationError::Ok && doc.containsKey("uid"))
      {
        const auto uid_str = doc["uid"].as<std::string>();
        // Check if the uid is present in the secrets::cards::whitelist
        const auto elem = std::find_if(secrets::cards::whitelist.begin(), secrets::cards::whitelist.end(),
                                       [&uid_str](const auto &elem)
                                       {
                                         const auto &[id, level, name] = elem;
                                         return card::uid_str(id) == uid_str;
                                       });
        if (elem != secrets::cards::whitelist.end())
        {
          std::stringstream ss;
          const auto &[id, level, name] = *elem;
          ss << "{\"request_ok\":true,\"is_valid\":" << (level != FabUser::UserLevel::Unknown ? "true" : "false")
             << ",\"level\":" << +static_cast<uint8_t>(level)
             << ",\"name\":\"" << name << "\"}";
          return ss.str();
        }

        // Still return a valid user
        std::stringstream ss;
        ss << "{\"request_ok\":true,\"is_valid\":true,\"level\":" << +2
           << ",\"name\":\"User" << uid_str << "\"}";
        return ss.str();
      }
      else
      {
        ESP_LOGE(TAG2, "Failed to parse checkuser query");
        return "{\"request_ok\":false}";
      }
    }

    if (query.find("alive") != std::string::npos)
    {
      return ""; // No reply to alive message
    }

    if (query.find(conf::default_config::mqtt_switch_topic) != std::string::npos) // Shelly doesn't reply
    {
      return "";
    }

    return std::string{"{\"request_ok\":true}"};
  }

  auto MockMQTTBroker::configureReplies(std::function<const std::string(const std::string &, const std::string &)> callback) -> void
  {
    std::lock_guard<std::mutex> lock(mutex);
    this->callback = callback;
  }

  auto MockMQTTBroker::processQueries() -> size_t
  {
    std::lock_guard<std::mutex> lock(mutex);

    if (!queries.empty())
    {
      std::string response{""};
      const auto [topic, query, reply_topic] = queries.front();
      queries.pop();
      response = callback(topic, query);

      if (!response.empty())
      {
        ESP_LOGI(TAG2, "MQTT BROKER: Sending %s -> %s", reply_topic.c_str(), response.c_str());
        publish(reply_topic, response);
      }
    }
    return queries.size();
  }

  void MockMQTTBroker::mainLoop()
  {
    // Check if the server is online
    if (!isRunning())
    {
      start();
    }
    else
    {
      update();
      processQueries();
    }
  }
} // namespace fabomatic