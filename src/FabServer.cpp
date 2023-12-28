#include "FabServer.hpp"
#include "secrets.hpp"

#include <string>
#include <string_view>
#include <cstdint>
#include <sstream>
#include <ArduinoJson.h>
#include <chrono>
#include "SavedConfig.hpp"

namespace fablabbg
{

  using namespace ServerMQTT;

  FabServer::FabServer()
  {
    char client_name[16]{0};
    std::snprintf(client_name, sizeof(client_name), "BOARD%ld", random(0, 1000));
    this->mqtt_client_name = client_name;
  }

  void FabServer::configure(const SavedConfig &config)
  {
    this->wifi_ssid = config.ssid;
    this->wifi_password = config.password;
    this->server_ip = config.mqtt_server;
    this->mqtt_user = config.mqtt_user;
    this->mqtt_password = config.mqtt_password;

#if (WOKWI_SIMULATION)
    this->channel = 6;
#else
    this->channel = -1;
#endif
    this->online = false;

    std::stringstream ss;
    ss << conf::mqtt::topic << "/" << config.machine_id;
    this->topic = ss.str();
  }

  /// @brief Posts to MQTT server and waits for answer
  /// @param query query to be posted
  /// @return true if the server answered
  bool FabServer::publishWithReply(const Query &query)
  {
    if (this->publish(query))
    {
      if (this->waitForAnswer())
      {
        if (conf::debug::ENABLE_LOGS)
          Serial.printf("MQTT Client: received answer: %s\r\n", this->last_reply.data());
        return true;
      }
    }
    return false;
  }

  /// @brief publish on MQTT the requested info
  /// @param mqtt_topic topic to publish to
  /// @param mqtt_payload payload to publish
  /// @return true if successfull
  bool FabServer::publish(String mqtt_topic, String mqtt_payload)
  {
    if (mqtt_payload.length() > FabServer::MAX_MQTT_LENGTH)
    {
      Serial.printf("MQTT Client: Message is too long: %s\r\n", mqtt_payload.c_str());
      return false;
    }

    this->answer_pending = true;
    this->last_query.assign(mqtt_payload.c_str());

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("MQTT Client: sending message %s on topic %s\r\n", mqtt_payload.c_str(), mqtt_topic.c_str());

    return this->client.publish(mqtt_topic.c_str(), mqtt_payload.c_str());
  }

  /// @brief posts to MQTT server
  /// @param query message to post
  /// @return true if the message was published
  bool FabServer::publish(const Query &query)
  {
    String s_payload(query.payload().data());
    String s_topic(this->topic.c_str());

    if (s_payload.length() > FabServer::MAX_MQTT_LENGTH)
    {
      Serial.printf("MQTT Client: Message is too long: %s\r\n", s_payload.c_str());
      return false;
    }

    this->answer_pending = true;
    this->last_query.assign(s_payload.c_str());

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("MQTT Client: sending message %s on topic %s\r\n", s_payload.c_str(), s_topic.c_str());

    return this->client.publish(s_topic, s_payload);
  }

  bool FabServer::loop()
  {
    if (!this->client.loop())
    {
      if (this->online && conf::debug::ENABLE_LOGS)
      {
        Serial.println("MQTT Client: connection lost");
      }
      this->online = false;
      return false;
    }
    return true;
  }

  /// @brief blocks until the server answers or until the timeout is reached
  /// @return true if the server answered
  bool FabServer::waitForAnswer()
  {
    constexpr auto MAX_DURATION_MS = 2000;
    constexpr auto DELAY_MS = 50;
    constexpr auto NB_TRIES = (MAX_DURATION_MS / DELAY_MS);

    for (auto i = 0; i < NB_TRIES; i++)
    {
      if (this->answer_pending)
      {
        this->client.loop();
        delay(DELAY_MS);
      }
      else
      {
        return true;
      }
    }
    Serial.printf("Failure, no answer from MQTT server (timeout:%d ms)\r\n", MAX_DURATION_MS);
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
    if (conf::debug::ENABLE_LOGS)
    {
      std::stringstream ss;
      ss << "MQTT Client: Received " << s_topic.c_str() << " -> " << s_payload.c_str();
      Serial.println(ss.str().c_str());
    }

    this->last_reply.assign(s_payload.c_str());
    this->answer_pending = false;
  }

  /// @brief Connects to the WiFi network
  /// @return true if the connection succeeded
  bool FabServer::connectWiFi() noexcept
  {
    static constexpr auto NB_TRIES = 3;
    static constexpr auto DELAY_MS = 1000;

    try
    {
      // Connect WiFi if needed
      if (this->WiFiConnection.status() != WL_CONNECTED)
      {
        if (conf::debug::ENABLE_LOGS)
          Serial.printf("FabServer::connectWiFi() : WiFi connection state=%d\n\r", this->WiFiConnection.status());

        this->WiFiConnection.begin(this->wifi_ssid.data(), this->wifi_password.data(), this->channel);
        for (auto i = 0; i < NB_TRIES; i++)
        {
          if (conf::debug::ENABLE_LOGS && this->WiFiConnection.status() == WL_CONNECTED)
            Serial.println("FabServer::connectWiFi() : WiFi connection successfull");
          break;
          delay(DELAY_MS);
        }
      }

      return this->WiFiConnection.status() == WL_CONNECTED;
    }
    catch (const std::exception &e)
    {
      Serial.println(e.what());
      return false;
    }
  }

  /// @brief Establish WiFi connection and connects to FabServer
  /// @return true if both operations succeeded
  bool FabServer::connect()
  {
    auto status = this->WiFiConnection.status();

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("FabServer::connect() called, Wifi status=%d\r\n", status);

    // Check if WiFi nextwork is available, and if not, try to connect
    if (status != WL_CONNECTED)
    {
      this->online = false;

      if (this->client.connected())
      {
        if (conf::debug::ENABLE_LOGS)
          Serial.printf("Closing MQTT client due to WiFi down\r\n");
        this->client.disconnect();
      }

      this->connectWiFi();
    }

    // Check if WiFi is available but MQTT client is not
    if (this->WiFiConnection.status() == WL_CONNECTED &&
        !this->client.connected())
    {
      if (conf::debug::ENABLE_LOGS)
        Serial.printf("Connecting to MQTT server %s\r\n", this->server_ip.c_str());

      this->client.begin(this->server_ip.data(), this->net);

      this->callback = [&](String &a, String &b)
      { return this->messageReceived(a, b); };

      this->client.onMessage(this->callback);

      if (!client.connect(this->mqtt_client_name.c_str(),
                          this->mqtt_user.c_str(),
                          this->mqtt_password.c_str(), false))
      {
        Serial.printf("Failure to connect as client: %s\r\n", mqtt_client_name.c_str());
      }

      // Setup subscriptions
      if (client.connected())
      {
        std::stringstream tmp_topic;
        tmp_topic << this->topic << conf::mqtt::response_topic;
        this->response_topic.assign(tmp_topic.str());

        if (!client.subscribe(this->response_topic.c_str()))
        {
          Serial.printf("MQTT Client: failure to subscribe to reply topic %s\r\n", this->response_topic.c_str());
        }
        else
        {
          this->online = true;
        }
      }
      else
      {
        Serial.printf("Failure to connect to MQTT server %s\r\n", this->server_ip.c_str());
      }
    }

    if (!client.connected())
    {
      this->online = false;
    }

    return this->online;
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
      if (this->isOnline())
      {
        if (QueryT query{args...}; this->publishWithReply(query))
        {
          // Deserialize the JSON document
          auto payload = this->last_reply.c_str();
          if (DeserializationError error = deserializeJson(this->doc, payload))
          {
            Serial.printf("Failed to parse json: %s (%s)", payload, error.c_str());
            throw std::runtime_error("Failed to parse json");
          }
          return RespT::fromJson(this->doc);
        }
      }
    }
    catch (const std::exception &e)
    {
      Serial.println(e.what());
    }
    return std::make_unique<RespT>(false);
  }

  /// @brief Checks if the card ID is known to the server
  /// @param uid card uid
  /// @return server response (if request_ok)
  std::unique_ptr<UserResponse> FabServer::checkCard(card::uid_t uid)
  {
    return this->processQuery<UserResponse, UserQuery>(uid);
  }

  /// @brief Checks the machine status on the server
  /// @param mid machine id
  /// @return server response (if request_ok)
  std::unique_ptr<MachineResponse> FabServer::checkMachine()
  {
    return this->processQuery<MachineResponse, MachineQuery>();
  }

  /// @brief register the starting of a machine usage
  /// @param uid card uid
  /// @param mid machine id
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::startUse(card::uid_t uid)
  {
    return this->processQuery<SimpleResponse, StartUseQuery>(uid);
  }

  /// @brief Register end of machine usage
  /// @param uid card ID of the machine user
  /// @param mid machine used ID
  /// @param duration_s duration of usage in seconds
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::finishUse(card::uid_t uid, std::chrono::seconds duration_s)
  {
    return this->processQuery<SimpleResponse, StopUseQuery>(uid, duration_s);
  }

  /// @brief Registers a maintenance action
  /// @param maintainer who performed the maintenance
  /// @param mid machine maintenance done
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::registerMaintenance(card::uid_t maintainer)
  {
    return this->processQuery<SimpleResponse, RegisterMaintenanceQuery>(maintainer);
  }

  /// @brief Sends a ping to the server
  /// @param mid machine ID
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::alive()
  {
    return this->processQuery<SimpleResponse, AliveQuery>();
  }
} // namespace fablabbg