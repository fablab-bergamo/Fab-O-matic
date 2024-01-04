#include "FabServer.hpp"
#include "secrets.hpp"

#include <string>
#include <string_view>
#include <cstdint>
#include <sstream>
#include <ArduinoJson.h>
#include <chrono>
#include "SavedConfig.hpp"
#include "Logging.hpp"

namespace fablabbg
{

  using namespace ServerMQTT;

  void FabServer::configure(const SavedConfig &config)
  {
    wifi_ssid = config.ssid;
    wifi_password = config.password;
    server_ip = config.mqtt_server;
    mqtt_user = config.mqtt_user;
    mqtt_password = config.mqtt_password;

#if (WOKWI_SIMULATION)
    channel = 6;
#else
    channel = -1;
#endif
    online = false;

    std::stringstream ss;
    ss << conf::mqtt::topic << "/" << config.machine_id;
    topic = ss.str();

    char client_name[16]{0};
    std::snprintf(client_name, sizeof(client_name), "BOARD%ld", random(0, 1000));
    mqtt_client_name = client_name;

    if (client.connected()) // Topic or IP may also have changed
    {
      client.disconnect();
      wifi_client.stop();
    }
    ESP_LOGD(TAG, "FabServer configured");
  }

  /// @brief Posts to MQTT server and waits for answer
  /// @param query query to be posted
  /// @return true if the server answered
  bool FabServer::publishWithReply(const Query &query)
  {
    constexpr auto MAX_TRIES = 3;
    constexpr auto TIMEOUT_SERVER = 500ms;
    static_assert(MAX_TRIES * TIMEOUT_SERVER < conf::mqtt::MAX_RETRY_DURATION, "MAX_RETRY_DURATION is too short");
    constexpr auto SLEEP_MS = duration_cast<milliseconds>(conf::mqtt::MAX_RETRY_DURATION + MAX_TRIES * TIMEOUT_SERVER).count() / MAX_TRIES;
    auto try_cpt = 0;
    while (try_cpt < MAX_TRIES)
    {
      if (publish(query))
      {
        if (waitForAnswer(TIMEOUT_SERVER))
        {
          ESP_LOGD(TAG, "MQTT Client: received answer: %s", last_reply.data());
          return true;
        }
        else
        {
          ESP_LOGW(TAG, "MQTT Client: no answer received, retrying %d/%d", (try_cpt + 1), MAX_TRIES);
          delay(SLEEP_MS);
        }
      }
      try_cpt++;
    }
    ESP_LOGE(TAG, "MQTT Client: failure to send query %s", query.payload().data());
    return false;
  }

  /// @brief publish on MQTT the requested info
  /// @param mqtt_topic topic to publish to
  /// @param mqtt_payload payload to publish
  /// @return true if successfull
  bool FabServer::publish(String mqtt_topic, String mqtt_payload)
  {
    if (mqtt_payload.length() + mqtt_topic.length() > FabServer::MAX_MSG_SIZE - 8)
    {
      ESP_LOGE(TAG, "MQTT Client: Message is too long: %s", mqtt_payload.c_str());
      return false;
    }

    answer_pending = true;
    last_query.assign(mqtt_payload.c_str());

    ESP_LOGI(TAG, "MQTT Client: sending message %s on topic %s", mqtt_payload.c_str(), mqtt_topic.c_str());

    return client.publish(mqtt_topic.c_str(), mqtt_payload.c_str());
  }

  /// @brief posts to MQTT server
  /// @param query message to post
  /// @return true if the message was published
  bool FabServer::publish(const Query &query)
  {
    String s_payload(query.payload().data());
    String s_topic(topic.c_str());

    if (s_payload.length() + topic.length() > FabServer::MAX_MSG_SIZE - 8)
    {
      ESP_LOGE(TAG, "MQTT Client: Message is too long: %s", s_payload.c_str());
      return false;
    }

    answer_pending = true;
    last_query.assign(s_payload.c_str());

    ESP_LOGD(TAG, "MQTT Client: sending message %s on topic %s", s_payload.c_str(), s_topic.c_str());

    return client.publish(s_topic, s_payload);
  }

  bool FabServer::loop()
  {
    if (!client.loop())
    {
      if (online)
      {
        ESP_LOGI(TAG, "MQTT Client: connection lost");
      }
      online = false;
      return false;
    }
    return true;
  }

  /// @brief blocks until the server answers or until the timeout is reached
  /// @return true if the server answered
  bool FabServer::waitForAnswer(milliseconds max_duration)
  {
    const auto MAX_DURATION_MS = max_duration.count();
    const auto DELAY_MS{50};
    const auto NB_TRIES{MAX_DURATION_MS / DELAY_MS};

    for (auto i = 0; i < NB_TRIES; i++)
    {
      if (answer_pending)
      {
        client.loop();
        if (!client.connected())
        {
          ESP_LOGW(TAG, "MQTT Client: connection lost while waiting for answer");
          connect();
        }
        delay(DELAY_MS);
      }
      else
      {
        return true;
      }
    }
    ESP_LOGE(TAG, "Failure, no answer from MQTT server (timeout:%d ms)", MAX_DURATION_MS);
    return false;
  }

  /// @brief true if the server has been reached successfully
  /// @return boolean
  bool FabServer::isOnline() const
  {
    return online;
  }

  /// @brief Callback for MQTT messages
  /// @param topic topic the message was received on
  /// @param payload payload of the message
  void FabServer::messageReceived(String &s_topic, String &s_payload)
  {
    ESP_LOGI(TAG, "MQTT Client: Received on %s -> %s", s_topic.c_str(), s_payload.c_str());

    last_reply.assign(s_payload.c_str());
    answer_pending = false;
  }

  /// @brief Connects to the WiFi network
  /// @return true if the connection succeeded
  bool FabServer::connectWiFi() noexcept
  {
    static constexpr auto NB_TRIES = 30;
    static constexpr auto DELAY_MS = 100;

    try
    {
      // Connect WiFi if needed
      if (WiFi.status() != WL_CONNECTED)
      {
        WiFi.mode(WIFI_STA);
        ESP_LOGD(TAG, "FabServer::connectWiFi() : WiFi connection state=%d, connecting to SSID:%s (channel:%d)", WiFi.status(), wifi_ssid.c_str(), channel);
        WiFi.begin(wifi_ssid.data(), wifi_password.data(), channel);
        delay(DELAY_MS);
        for (auto i = 0; i < NB_TRIES; i++)
        {
          if (WiFi.status() == WL_CONNECTED)
          {
            ESP_LOGD(TAG, "FabServer::connectWiFi() : WiFi connection successfull");
            break;
          }
          delay(DELAY_MS);
        }
      }
      return WiFi.status() == WL_CONNECTED;
    }
    catch (const std::exception &e)
    {
      ESP_LOGE(TAG, "connectWiFi exception %s", e.what());
      return false;
    }
  }

  /// @brief Establish WiFi connection and connects to FabServer
  /// @return true if both operations succeeded
  bool FabServer::connect()
  {
    auto status = WiFi.status();

    ESP_LOGD(TAG, "FabServer::connect() called, Wifi status=%d", status);

    // Check if WiFi nextwork is available, and if not, try to connect
    if (status != WL_CONNECTED)
    {
      online = false;

      if (client.connected())
      {
        ESP_LOGD(TAG, "Closing MQTT client due to WiFi down");
        client.disconnect();
      }

      connectWiFi();
    }

    // Check if WiFi is available but MQTT client is not
    if (WiFi.status() == WL_CONNECTED &&
        !client.connected())
    {
      ESP_LOGD(TAG, "Connecting to MQTT server %s", server_ip.c_str());
      client.begin(server_ip.data(), wifi_client);

      callback = [&](String &a, String &b)
      { return messageReceived(a, b); };

      client.onMessage(callback);

      if (!client.connect(mqtt_client_name.c_str(),
                          mqtt_user.c_str(),
                          mqtt_password.c_str(), false))
      {
        ESP_LOGW(TAG, "Failure to connect as client: %s with username %s, last error %d", mqtt_client_name.c_str(), mqtt_user.c_str(), client.lastError());
      }

      // Setup subscriptions
      if (client.connected())
      {
        std::stringstream tmp_topic;
        tmp_topic << topic << conf::mqtt::response_topic;
        response_topic.assign(tmp_topic.str());

        if (!client.subscribe(response_topic.c_str()))
        {
          ESP_LOGE(TAG, "MQTT Client: failure to subscribe to reply topic %s", response_topic.c_str());
        }
        else
        {
          ESP_LOGD(TAG, "MQTT Client: subscribed to reply topic %s", response_topic.c_str());
          online = true;
        }
      }
      else
      {
        ESP_LOGW(TAG, "Failure to connect to MQTT server %s", server_ip.c_str());
      }
    }

    if (!client.connected())
    {
      online = false;
    }

    return online;
  }

  /// @brief Disconnects from the server
  void FabServer::disconnect()
  {
    client.disconnect();
  }

  /// @brief Process a MQTT query and returns the response
  /// @tparam T type of the response returned to the caller (it will be wrapped in a unique_ptr)
  /// @tparam Q type of the query sent to the server
  /// @tparam ...Args arguments to be passed to the constructor of Q
  /// @return server response (if request_ok)
  template <typename RespT, typename QueryT, typename... QueryArgs>
  std::unique_ptr<RespT> FabServer::processQuery(QueryArgs &&...args)
  {
    static_assert(std::is_base_of<ServerMQTT::Query, QueryT>::value, "QueryT must inherit from Query");
    static_assert(std::is_base_of<ServerMQTT::Response, RespT>::value, "RespT must inherit from Response");
    try
    {
      if (isOnline())
      {
        if (QueryT query{args...}; publishWithReply(query))
        {
          // Deserialize the JSON document
          auto payload = last_reply.c_str();
          if (DeserializationError error = deserializeJson(doc, payload))
          {
            ESP_LOGE(TAG, "Failed to parse json: %s (%s)", payload, error.c_str());
            throw std::runtime_error("Failed to parse json");
          }
          return RespT::fromJson(doc);
        }
      }
    }
    catch (const std::exception &e)
    {
      ESP_LOGE(TAG, "processQuery exception %s", e.what());
    }
    return std::make_unique<RespT>(false);
  }

  /// @brief Checks if the card ID is known to the server
  /// @param uid card uid
  /// @return server response (if request_ok)
  std::unique_ptr<UserResponse> FabServer::checkCard(card::uid_t uid)
  {
    return processQuery<UserResponse, UserQuery>(uid);
  }

  /// @brief Checks the machine status on the server
  /// @return server response (if request_ok)
  std::unique_ptr<MachineResponse> FabServer::checkMachine()
  {
    return processQuery<MachineResponse, MachineQuery>();
  }

  /// @brief register the starting of a machine usage
  /// @param uid card uid
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::startUse(card::uid_t uid)
  {
    return processQuery<SimpleResponse, StartUseQuery>(uid);
  }

  /// @brief Register end of machine usage
  /// @param uid card ID of the machine user
  /// @param duration_s duration of usage in seconds
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::finishUse(card::uid_t uid, std::chrono::seconds duration_s)
  {
    return processQuery<SimpleResponse, StopUseQuery>(uid, duration_s);
  }

  /// @brief Registers a maintenance action
  /// @param maintainer who performed the maintenance
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::registerMaintenance(card::uid_t maintainer)
  {
    return processQuery<SimpleResponse, RegisterMaintenanceQuery>(maintainer);
  }

  /// @brief Sends a ping to the server
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::alive()
  {
    return processQuery<SimpleResponse, AliveQuery>();
  }

  /// @brief set channel to use for WiFi connection
  /// @param channel
  void FabServer::setChannel(int32_t channel)
  {
    this->channel = channel;
  }
} // namespace fablabbg