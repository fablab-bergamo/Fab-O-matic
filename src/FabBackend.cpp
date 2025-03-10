#include "FabBackend.hpp"
#include "secrets.hpp"
#include "Logging.hpp"
#include "SavedConfig.hpp"
#include "Tasks.hpp"
#include <ArduinoJson.h>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <array>

namespace fabomatic
{
  /**
   * @brief Configures the FabBackend with the given configuration.
   *
   * @param config The SavedConfig object containing the configuration parameters.
   */
  void FabBackend::configure(const SavedConfig &config)
  {
    wifi_ssid = config.ssid;
    wifi_password = config.password;
    broker_hostname = config.mqtt_server;
    mqtt_user = config.mqtt_user;
    mqtt_password = config.mqtt_password;

#if (PINS_WOKWI)
    channel = 6;
    broker_hostname = "localhost";
#else
    channel = -1;
#endif
    mqtt_connected = false;

    std::stringstream ss_topic_name, ss_client_name;
    ss_topic_name << conf::mqtt::topic << "/" << config.machine_id;
    topic = ss_topic_name.str();

    ss_client_name << "BOARD" << random(0, 1000);
    mqtt_client_name = ss_client_name.str();

    if (client.connected()) // Topic or IP may also have changed
    {
      client.disconnect();
      wifi_client.stop();
    }

    loadBuffer(config.message_buffer);

    ESP_LOGD(TAG, "FabServer configured");
  }

  /**
   * @brief Posts a query to the MQTT server and waits for a reply.
   *
   * @param query The query to be posted.
   * @return true if the server answered, false otherwise.
   */
  auto FabBackend::publishWithReply(const MQTTInterface::Query &query) -> PublishResult
  {
    auto try_cpt = 0;
    auto published = false;

    while (try_cpt < conf::mqtt::MAX_TRIES)
    {
      try_cpt++;

      if (!published)
      {
        if (publish(query) == PublishResult::PublishedWithoutAnswer)
        {
          published = true;
        }
        else
        {
          ESP_LOGE(TAG, "MQTT Client: failure to send query %s", query.payload().data());
          Tasks::delay(conf::mqtt::TIMEOUT_REPLY_SERVER);
        }
      }

      if (waitForAnswer(conf::mqtt::TIMEOUT_REPLY_SERVER))
      {
        ESP_LOGD(TAG, "MQTT Client: received answer: %s", last_reply.data());
        last_unresponsive = std::nullopt;
        return PublishResult::PublishedWithAnswer;
      }

      ESP_LOGW(TAG, "MQTT Client: no answer received, retrying %d/%d", try_cpt, conf::mqtt::MAX_TRIES);
      Tasks::delay(conf::mqtt::TIMEOUT_REPLY_SERVER);
    }

    ESP_LOGE(TAG, "MQTT Client: failure to send query %s", query.payload().data());

    // Do not send twice if response did not arrive
    if (query.buffered() && !published)
    {
      const auto &msg = BufferedMsg{query.payload(), topic, query.waitForReply()};
      buffer.push_back(msg);
    }

    if (published)
    {
      last_unresponsive = Tasks::arduinoNow();
      return PublishResult::PublishedWithoutAnswer;
    }

    return PublishResult::ErrorNotPublished;
  }

  /**
   * @brief Publishes a message on the MQTT server.
   *
   * @param mqtt_topic The topic to publish to.
   * @param mqtt_payload The payload to publish.
   * @param answerNeeded Whether to wait for an answer.
   * @return true if the message was published successfully, false otherwise.
   */
  bool FabBackend::publish(String mqtt_topic, String mqtt_payload, bool answerNeeded)
  {
    if (mqtt_payload.length() + mqtt_topic.length() > FabBackend::MAX_MSG_SIZE - 8)
    {
      ESP_LOGE(TAG, "MQTT Client: Message is too long: %s", mqtt_payload.c_str());
      return false;
    }

    answer_pending = answerNeeded;
    last_query.assign(mqtt_payload.c_str());

    ESP_LOGI(TAG, "MQTT Client: sending message %s on topic %s", mqtt_payload.c_str(), mqtt_topic.c_str());

    return client.publish(mqtt_topic.c_str(), mqtt_payload.c_str());
  }

  /**
   * @brief Publishes a query on the MQTT server.
   *
   * @param query The query to be published.
   * @return true if the query was published successfully, false otherwise.
   */
  template <typename QueryT>
  auto FabBackend::publish(const QueryT &query) -> PublishResult
  {
    String s_payload(query.payload().c_str());
    std::string temp_topic;

    // Check at compile-time if topic is specified
    if constexpr (std::is_base_of<BufferedQuery, QueryT>::value)
    {
      temp_topic = static_cast<BufferedQuery>(query).topic();
    }
    else
    {
      // Just use the default topic for the machine
      temp_topic = this->topic;
    }

    String s_topic{temp_topic.c_str()};

    if (s_payload.length() + s_topic.length() > FabBackend::MAX_MSG_SIZE - 8)
    {
      ESP_LOGE(TAG, "MQTT Client: Message is too long: %s", s_payload.c_str());
      return PublishResult::ErrorNotPublished;
    }

    answer_pending = query.waitForReply(); // Don't wait if not needed
    last_query.assign(s_payload.c_str());
    last_reply.clear();

    ESP_LOGD(TAG, "MQTT Client: sending message %s on topic %s", s_payload.c_str(), s_topic.c_str());

    if (client.publish(s_topic, s_payload))
    {
      return PublishResult::PublishedWithoutAnswer;
    }

    return PublishResult::ErrorNotPublished;
  }

  /**
   * @brief Main loop for the MQTT client.
   *
   * @return true if the client is running successfully, false otherwise.
   */
  bool FabBackend::loop()
  {
    if (!client.loop())
    {
      if (mqtt_connected)
      {
        ESP_LOGI(TAG, "MQTT Client: connection lost");
      }
      mqtt_connected = false;
      return false;
    }
    return true;
  }

  /**
   * @brief Waits for an answer from the MQTT server.
   *
   * @param max_duration The maximum duration to wait.
   * @return true if the server answered, false otherwise.
   */
  bool FabBackend::waitForAnswer(std::chrono::milliseconds max_duration)
  {
    const auto start_time = Tasks::arduinoNow();
    const auto DELAY_MS = 25ms;
    do
    {
      if (answer_pending)
      {
        client.loop();
        if (!client.connected())
        {
          ESP_LOGW(TAG, "MQTT Client: connection lost while waiting for answer");
          connect();
        }
        Tasks::delay(DELAY_MS);
      }
      else
      {
        return true;
      }
    } while (Tasks::arduinoNow() < (start_time + max_duration));

    ESP_LOGE(TAG, "Failure, no answer from MQTT server (timeout:%" PRId64 " ms)", max_duration.count());

    return false;
  }

  /**
   * @brief Indicates if the MQTT client is connected to the broker and backend is responsive.
   *
   * @return true if the client is online, false otherwise.
   */
  bool FabBackend::isOnline() const
  {
    return mqtt_connected && isResponsive();
  }

  /**
   * @brief Indicates if backend answers are received
   *
   * @return true if the backend is responding, false otherwise.
   */
  bool FabBackend::isResponsive() const
  {
    return !last_unresponsive.has_value();
  }

  /**
   * @brief Callback function for received MQTT messages.
   *
   * @param s_topic The topic the message was received on.
   * @param s_payload The payload of the message.
   */
  void FabBackend::messageReceived(String &s_topic, String &s_payload)
  {
    ESP_LOGI(TAG, "MQTT Client: message received on %s -> %s", s_topic.c_str(), s_payload.c_str());

    // Needed for equality test below
    std::string_view view_topic{s_topic.c_str()};
    if (view_topic == this->response_topic)
    {
      last_reply.assign(s_payload.c_str());
      answer_pending = false;
    }
    else if (view_topic == this->request_topic)
    {
      last_request.assign(s_payload.c_str());
    }
    else if (view_topic != this->topic)
    {
      ESP_LOGW(TAG, "MQTT Client: unrecognized topic %s", s_topic.c_str());
    }
  }

  /**
   * @brief Connects to the WiFi network.
   *
   * @return true if the connection succeeded, false otherwise.
   */
  bool FabBackend::connectWiFi()
  {
    static constexpr auto NB_TRIES = 15;
    static constexpr auto DELAY_MS = 250ms;

    // Connect WiFi if needed
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.setAutoReconnect(true);
      WiFi.persistent(true);
      WiFi.mode(WIFI_STA);
      ESP_LOGD(TAG, "FabServer::connectWiFi() : WiFi connection state=%d, connecting to SSID:%s (channel:%d)", WiFi.status(), wifi_ssid.c_str(), channel);
      WiFi.begin(wifi_ssid.data(), wifi_password.data(), channel);
      Tasks::delay(DELAY_MS);
      for (auto i = 0; i < NB_TRIES; i++)
      {
        if (WiFi.status() == WL_CONNECTED)
        {
          ESP_LOGD(TAG, "FabServer::connectWiFi() : WiFi connection successful");
          break;
        }
        Tasks::delay(DELAY_MS);
      }
    }
    return WiFi.status() == WL_CONNECTED;
  }

  /**
   * @brief Establishes a connection to the WiFi network and the MQTT server.
   *
   * @return true if both operations succeeded, false otherwise.
   */
  bool FabBackend::connect()
  {
    const auto status = WiFi.status();

    ESP_LOGD(TAG, "FabServer::connect() called, Wifi status=%d", status);

    // Check if WiFi network is available, and if not, try to connect
    if (status != WL_CONNECTED)
    {
      mqtt_connected = false;

      if (client.connected())
      {
        ESP_LOGD(TAG, "Closing MQTT client due to WiFi down");
        client.disconnect();
      }
      Tasks::delay(100ms);
      connectWiFi();
    }

    // Check if WiFi is available but MQTT client is not
    if (WiFi.status() == WL_CONNECTED &&
        !client.connected())
    {
      IPAddress ip;
      if (WiFi.hostByName(broker_hostname.c_str(), ip))
      {
        ESP_LOGD(TAG, "Resolved MQTT server [%s] as [%s]", broker_hostname.c_str(), ip.toString().c_str());
      }
      else
      {
        ESP_LOGE(TAG, "Failed to resolve MQTT server [%s]", broker_hostname.c_str());
        return false;
      }

      ESP_LOGD(TAG, "Connecting to MQTT server [%s:%d]...", ip.toString().c_str(), conf::mqtt::PORT_NUMBER);

      client.begin(ip, conf::mqtt::PORT_NUMBER, wifi_client);

      callback = [&](String &a, String &b)
      { return messageReceived(a, b); };

      client.onMessage(callback);

      if (!client.connect(mqtt_client_name.c_str(),
                          mqtt_user.c_str(),
                          mqtt_password.c_str(), false))
      {
        ESP_LOGW(TAG, "Failure to connect as client: %s with username %s, last error %d", mqtt_client_name.c_str(), mqtt_user.c_str(), client.lastError());
        client.disconnect();
      }

      // Setup subscriptions
      if (client.connected())
      {
        std::stringstream ss_resp{};
        ss_resp << topic << conf::mqtt::response_topic;
        response_topic.assign(ss_resp.str());

        if (!client.subscribe(response_topic.c_str()))
        {
          ESP_LOGE(TAG, "MQTT Client: failure to subscribe to reply topic %s", response_topic.c_str());
        }
        else
        {
          ESP_LOGD(TAG, "MQTT Client: subscribed to reply topic %s", response_topic.c_str());
          mqtt_connected = true;
        }

        std::stringstream ss_req{};
        ss_req << topic << conf::mqtt::request_topic;
        request_topic.assign(ss_req.str());

        if (!client.subscribe(request_topic.c_str()))
        {
          ESP_LOGE(TAG, "MQTT Client: failure to subscribe to requests topic %s", request_topic.c_str());
        }
        else
        {
          ESP_LOGD(TAG, "MQTT Client: subscribed to requests topic %s", request_topic.c_str());
          mqtt_connected = true;
        }

        // Announce the board to the server
        if (auto query = MQTTInterface::AliveQuery{}; publish(query) == PublishResult::PublishedWithoutAnswer)
        {
          ESP_LOGI(TAG, "MQTT Client: board announced to server");
        }
        else
        {
          ESP_LOGW(TAG, "MQTT Client: failure to announce board to server");
        }
      }
      else
      {
        ESP_LOGW(TAG, "Failure to connect to MQTT server %s", broker_hostname.c_str());
      }
    }

    if (!client.connected())
    {
      mqtt_connected = false;
      last_unresponsive = Tasks::arduinoNow();
    }
    else
    {
      last_unresponsive = std::nullopt;
    }

    return mqtt_connected;
  }

  /**
   * @brief Disconnects from the MQTT server.
   */
  void FabBackend::disconnect()
  {
    client.disconnect();
    wifi_client.stop();
    Tasks::delay(100ms);
  }

  /**
   * @brief Processes a query and returns the response.
   *
   * @tparam RespT The type of the response.
   * @tparam QueryT The type of the query.
   * @tparam ...Args The arguments to be passed to the query constructor.
   * @return A unique_ptr to the response.
   */
  template <typename RespT, typename QueryT, typename... QueryArgs>
  std::unique_ptr<RespT> FabBackend::processQuery(QueryArgs &&...args)
  {
    static_assert(std::is_base_of<MQTTInterface::Query, QueryT>::value, "QueryT must inherit from Query");
    static_assert(std::is_base_of<MQTTInterface::Response, RespT>::value, "RespT must inherit from Response");
    QueryT query{args...};

    auto nb_tries = 0;
    while (isOnline() && hasBufferedMsg() && !transmitBuffer() && nb_tries < 3)
    {
      // To preserve chronological order, we cannot send new messages until the old ones have been sent.
      ESP_LOGW(TAG, "Online with pending messages that could not be transmitted, retrying...");

      if (!shouldFailFast())
      {
        connect();
      }
      else
      {
        ESP_LOGW(TAG, "processQuery: failing fast due to unresponsive backend.");
        break;
      }

      Tasks::delay(250ms);
      nb_tries++;
    }

    if (isOnline() && !hasBufferedMsg())
    {
      if (publishWithReply(query) == PublishResult::PublishedWithAnswer)
      {
        // Deserialize the JSON document
        const auto payload = last_reply.c_str();
        if (DeserializationError error = deserializeJson(doc, payload))
        {
          ESP_LOGE(TAG, "Failed to parse json: %s (%s)", payload, error.c_str());
          return std::make_unique<RespT>(false);
        }
        return RespT::fromJson(doc);
      }
      else
      {
        ESP_LOGE(TAG, "Failed to publish query %s", query.payload().data());
        this->disconnect();
      }
    }

    if (query.buffered())
    {
      const auto &msg = BufferedMsg{query.payload(), topic, query.waitForReply()};
      buffer.push_back(msg);
    }

    return std::make_unique<RespT>(false);
  }

  /**
   * @brief Processes a query.
   *
   * @tparam QueryT The type of the query.
   * @tparam ...Args The arguments to be passed to the query constructor.
   * @return true if the query was processed successfully, false otherwise.
   */
  template <typename QueryT, typename... QueryArgs>
  bool FabBackend::processQuery(QueryArgs &&...args)
  {
    static_assert(std::is_base_of<MQTTInterface::Query, QueryT>::value, "QueryT must inherit from Query");
    QueryT query{args...};

    auto nb_tries = 0;
    while (isOnline() && hasBufferedMsg() && !transmitBuffer() && nb_tries < 3)
    {
      // To preserve chronological order, we cannot send new messages until the old ones have been sent.
      ESP_LOGW(TAG, "Online with pending messages that could not be transmitted, retrying...");

      if (!isOnline())
      {
        if (!shouldFailFast())
        {
          connect();
        }
        else
        {
          ESP_LOGW(TAG, "processQuery: failing fast due to unresponsive backend.");
          break;
        }
      }

      Tasks::delay(250ms);
      nb_tries++;
    }

    if (isOnline() && !hasBufferedMsg())
    {
      if (publish(query) == PublishResult::PublishedWithoutAnswer)
      {
        return true;
      }

      ESP_LOGW(TAG, "Failed to publish query %s", query.payload().data());
      this->disconnect();
    }

    if (query.buffered())
    {
      const auto &msg = BufferedMsg{query.payload(), topic, query.waitForReply()};
      buffer.push_back(msg);
    }
    return false;
  }

  /**
   * @brief Checks if the card ID is known to the server.
   *
   * @param uid The card UID.
   * @return A unique_ptr to the server response.
   */
  std::unique_ptr<MQTTInterface::UserResponse> FabBackend::checkCard(card::uid_t uid)
  {
    return processQuery<MQTTInterface::UserResponse, MQTTInterface::UserQuery>(uid);
  }

  /**
   * @brief Checks the machine status on the server.
   *
   * @return A unique_ptr to the server response.
   */
  std::unique_ptr<MQTTInterface::MachineResponse> FabBackend::checkMachine()
  {
    return processQuery<MQTTInterface::MachineResponse, MQTTInterface::MachineQuery>();
  }

  /**
   * @brief Registers the start of machine usage.
   *
   * @param uid The card UID of the user.
   * @return A unique_ptr to the server response.
   */
  std::unique_ptr<MQTTInterface::SimpleResponse> FabBackend::startUse(card::uid_t uid)
  {
    return processQuery<MQTTInterface::SimpleResponse, MQTTInterface::StartUseQuery>(uid);
  }

  /**
   * @brief Registers the end of machine usage.
   *
   * @param uid The card UID of the user.
   * @param duration_s The duration of usage in seconds.
   * @return A unique_ptr to the server response.
   */
  std::unique_ptr<MQTTInterface::SimpleResponse> FabBackend::finishUse(card::uid_t uid, std::chrono::seconds duration_s)
  {
    return processQuery<MQTTInterface::SimpleResponse, MQTTInterface::StopUseQuery>(uid, duration_s);
  }

  /**
   * @brief Informs the backend that the machine is in use.
   *
   * @param uid The card UID of the user.
   * @param duration_s The duration of usage in seconds.
   * @return A unique_ptr to the server response.
   */
  std::unique_ptr<MQTTInterface::SimpleResponse> FabBackend::inUse(card::uid_t uid, std::chrono::seconds duration_s)
  {
    return processQuery<MQTTInterface::SimpleResponse, MQTTInterface::InUseQuery>(uid, duration_s);
  }

  /**
   * @brief Registers a maintenance action.
   *
   * @param maintainer The UID of the person performing maintenance.
   * @return A unique_ptr to the server response.
   */
  std::unique_ptr<MQTTInterface::SimpleResponse> FabBackend::registerMaintenance(card::uid_t maintainer)
  {
    return processQuery<MQTTInterface::SimpleResponse, MQTTInterface::RegisterMaintenanceQuery>(maintainer);
  }

  /**
   * @brief Sends a ping to the server.
   *
   * @return true if the ping was sent successfully, false otherwise.
   */
  bool FabBackend::alive()
  {
    return processQuery<MQTTInterface::AliveQuery>();
  }

  /**
   * @brief Sets the WiFi channel to use.
   *
   * @param channel The channel number.
   */
  void FabBackend::setChannel(int16_t channel)
  {
    this->channel = channel;
  }

  [[nodiscard]] auto FabBackend::hasBufferedMsg() const -> bool
  {
    return this->buffer.count() > 0;
  }

  [[nodiscard]] auto FabBackend::transmitBuffer() -> bool
  {
    while (hasBufferedMsg() && isOnline())
    {
      ESP_LOGD(TAG, "Retransmitting buffered messages...");
      const auto &msg = buffer.getMessage();
      const BufferedQuery bq{msg.mqtt_message, msg.mqtt_topic, msg.wait_for_answer};
      if (bq.waitForReply())
      {
        if (auto result = publishWithReply(bq); result != PublishResult::PublishedWithAnswer)
        {
          ESP_LOGW(TAG, "Retransmitting buffered message failed!");

          // If it has been published but not answered, do not try again
          if (result == PublishResult::PublishedWithoutAnswer)
          {
            const auto &bmsg = BufferedMsg{msg.mqtt_message, msg.mqtt_topic, msg.wait_for_answer};
            buffer.push_front(bmsg);
          }
          break;
        }
      }
      else
      {
        if (publish(bq) == PublishResult::ErrorNotPublished)
        {
          // Will try again
          ESP_LOGW(TAG, "Retransmitting buffered message failed!");
          const auto &bmsg = BufferedMsg{msg.mqtt_message, msg.mqtt_topic, msg.wait_for_answer};
          buffer.push_front(bmsg);
          break;
        }
      }
    }
    last_reply.clear();

    ESP_LOGW(TAG, "Retransmittion completed, remaining messages=%d", buffer.count());
    return !hasBufferedMsg();
  }

  auto FabBackend::saveBuffer() -> bool
  {
    if (!buffer.hasChanged())
    {
      return true;
    }

    auto sc = SavedConfig::LoadFromEEPROM().value_or(SavedConfig::DefaultConfig());
    sc.message_buffer = buffer;

    if (sc.SaveToEEPROM())
    {
      buffer.setChanged(false);
      ESP_LOGI(TAG, "Saved %d buffered messages", buffer.count());
      return true;
    }

    ESP_LOGE(TAG, "Failed to save buffered messages!");
    return false;
  }

  auto FabBackend::loadBuffer(const Buffer &new_buffer) -> void
  {
    buffer = new_buffer;
    buffer.setChanged(false);
    ESP_LOGI(TAG, "Loaded buffer with %d messages", buffer.count());
  }

  auto FabBackend::checkBackendRequest() -> std::optional<std::unique_ptr<MQTTInterface::BackendRequest>>
  {
    if (!this->last_request.empty())
    {
      const auto payload = last_request.c_str();
      if (DeserializationError error = deserializeJson(doc, payload))
      {
        ESP_LOGE(TAG, "Failed to parse json: %s (%s)", payload, error.c_str());
        last_request.clear();
        return std::nullopt;
      }

      last_request.clear();

      return MQTTInterface::BackendRequest::fromJson(doc);
    }
    return std::nullopt;
  }

  /**
   * @brief when server is unresponsive, wait for a configurable time before to try again
   */
  auto FabBackend::shouldFailFast() const -> bool
  {
    return this->mqtt_connected &&
           this->last_unresponsive.has_value() &&
           (Tasks::arduinoNow() - this->last_unresponsive.value()) <= conf::mqtt::FAIL_FAST_PERIOD;
  }

} // namespace fabomatic
