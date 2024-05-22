#include "mock/MockMQTTBroker.hpp"
#include "Logging.hpp"
#include "conf.hpp"
#include "secrets.hpp"

#include <ArduinoJson.h>

// TAG for logging purposes
static const char *const TAG2 = "MockMQTTBroker";

namespace fabomatic
{
  /**
   * @brief Starts the MQTT broker.
   *
   * This function checks if the Wi-Fi is connected and initializes the MQTT broker.
   * If the Wi-Fi is not connected, it logs the status and returns.
   * If the broker is not running, it initializes the broker with the configured port number.
   */
  auto MockMQTTBroker::start() -> void
  {
    // Wait for the Wi-Fi to connect
    while (WiFi.status() != WL_CONNECTED)
    {
      ESP_LOGD(TAG2, "MQTT BROKER: WiFi status changed to %d", WiFi.status());
      is_running = false;
      return;
    }
    // Initialize the broker if it's not running
    if (!is_running)
    {
      is_running = init(conf::mqtt::PORT_NUMBER, true);
      ESP_LOGI(TAG2, "MQTT BROKER: started with result %d", is_running);
    }
  }

  /**
   * @brief Handles MQTT events.
   *
   * @param event Pointer to the sMQTTEvent object containing event details.
   * @return true if the event was handled successfully, false otherwise.
   *
   * This function handles various types of MQTT events such as new client connections,
   * message publications, client removals, connection losses, subscriptions, and unsubscriptions.
   */
  auto MockMQTTBroker::onEvent(sMQTTEvent *event) -> bool
  {
    switch (event->Type())
    {
    case NewClient_sMQTTEventType:
    {
      // Handle new client connection event
      auto *e = static_cast<sMQTTNewClientEvent *>(event);
      ESP_LOGD(TAG2, "MQTT BROKER: client connected, id:%s", e->Client()->getClientId().c_str());
    }
    break;
    case Public_sMQTTEventType:
    {
      // Handle publish event
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
      // Handle client removal event
      auto *e = static_cast<sMQTTRemoveClientEvent *>(event);
      ESP_LOGD(TAG2, "MQTT BROKER: removed client id: %s", e->Client()->getClientId().c_str());
    }
    break;
    case LostConnect_sMQTTEventType:
    {
      // Handle lost connection event
      is_running = false;
      ESP_LOGD(TAG2, "MQTT BROKER: lost connection");
    }
    break;
    case Subscribe_sMQTTEventType:
    {
      // Handle subscription event
      auto *e = static_cast<sMQTTSubUnSubClientEvent *>(event);
      ESP_LOGD(TAG2, "MQTT BROKER: client %s subscribed to %s", e->Client()->getClientId().c_str(), e->Topic().c_str());
    }
    break;
    case UnSubscribe_sMQTTEventType:
    {
      // Handle unsubscription event
      auto *e = static_cast<sMQTTSubUnSubClientEvent *>(event);
      ESP_LOGD(TAG2, "MQTT BROKER: got unsubscribe from %s", e->Topic().c_str());
    }
    break;
    default:
      ESP_LOGD(TAG2, "MQTT BROKER: unhandled event %d", event->Type());
      break;
    }
    return true;
  }

  /**
   * @brief Checks if the MQTT broker is running.
   *
   * @return true if the broker is running, false otherwise.
   */
  auto MockMQTTBroker::isRunning() const -> bool
  {
    return is_running;
  }

  /**
   * @brief Provides fake server replies for testing purposes.
   *
   * @param query The query string to determine the appropriate response.
   * @return The JSON payload response as a string.
   */
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

      ESP_LOGE(TAG2, "Failed to parse checkuser query");
      return "{\"request_ok\":false}";
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

  /**
   * @brief Configures custom replies for MQTT queries.
   *
   * @param callback A function that takes a topic and query as input and returns the corresponding response.
   */
  auto MockMQTTBroker::configureReplies(std::function<const std::string(const std::string &, const std::string &)> callback) -> void
  {
    std::lock_guard<std::mutex> lock(mutex);
    this->callback = callback;
  }

  /**
   * @brief Processes pending MQTT queries.
   *
   * @return The number of remaining queries.
   *
   * This function processes the queries in the queue by invoking the configured callback function
   * and publishes the response to the corresponding reply topic.
   */
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

  /**
   * @brief Main loop for the MQTT broker.
   *
   * This function checks if the broker is running. If not, it starts the broker.
   * If the broker is running, it updates the broker state and processes pending queries.
   */
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
