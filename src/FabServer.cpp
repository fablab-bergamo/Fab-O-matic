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
    }
  }

  /// @brief Posts to MQTT server and waits for answer
  /// @param query query to be posted
  /// @return true if the server answered
  bool FabServer::publishWithReply(const Query &query)
  {
    if (publish(query))
    {
      if (waitForAnswer())
      {
        if (conf::debug::ENABLE_LOGS)
          Serial.printf("MQTT Client: received answer: %s\r\n", last_reply.data());
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

    answer_pending = true;
    last_query.assign(mqtt_payload.c_str());

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("MQTT Client: sending message %s on topic %s\r\n", mqtt_payload.c_str(), mqtt_topic.c_str());

    return client.publish(mqtt_topic.c_str(), mqtt_payload.c_str());
  }

  /// @brief posts to MQTT server
  /// @param query message to post
  /// @return true if the message was published
  bool FabServer::publish(const Query &query)
  {
    String s_payload(query.payload().data());
    String s_topic(topic.c_str());

    if (s_payload.length() > FabServer::MAX_MQTT_LENGTH)
    {
      Serial.printf("MQTT Client: Message is too long: %s\r\n", s_payload.c_str());
      return false;
    }

    answer_pending = true;
    last_query.assign(s_payload.c_str());

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("MQTT Client: sending message %s on topic %s\r\n", s_payload.c_str(), s_topic.c_str());

    return client.publish(s_topic, s_payload);
  }

  bool FabServer::loop()
  {
    if (!client.loop())
    {
      if (online && conf::debug::ENABLE_LOGS)
      {
        Serial.println("MQTT Client: connection lost");
      }
      online = false;
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
      if (answer_pending)
      {
        client.loop();
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

    last_reply.assign(s_payload.c_str());
    answer_pending = false;
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
      if (WiFiConnection.status() != WL_CONNECTED)
      {
        if (conf::debug::ENABLE_LOGS)
          Serial.printf("FabServer::connectWiFi() : WiFi connection state=%d\n\r", WiFiConnection.status());

        WiFiConnection.begin(wifi_ssid.data(), wifi_password.data(), channel);
        for (auto i = 0; i < NB_TRIES; i++)
        {
          if (conf::debug::ENABLE_LOGS && WiFiConnection.status() == WL_CONNECTED)
            Serial.println("FabServer::connectWiFi() : WiFi connection successfull");
          break;
          delay(DELAY_MS);
        }
      }

      return WiFiConnection.status() == WL_CONNECTED;
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
    auto status = WiFiConnection.status();

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("FabServer::connect() called, Wifi status=%d\r\n", status);

    // Check if WiFi nextwork is available, and if not, try to connect
    if (status != WL_CONNECTED)
    {
      online = false;

      if (client.connected())
      {
        if (conf::debug::ENABLE_LOGS)
          Serial.printf("Closing MQTT client due to WiFi down\r\n");
        client.disconnect();
      }

      connectWiFi();
    }

    // Check if WiFi is available but MQTT client is not
    if (WiFiConnection.status() == WL_CONNECTED &&
        !client.connected())
    {
      if (conf::debug::ENABLE_LOGS)
        Serial.printf("Connecting to MQTT server %s\r\n", server_ip.c_str());

      client.begin(server_ip.data(), net);

      callback = [&](String &a, String &b)
      { return messageReceived(a, b); };

      client.onMessage(callback);

      if (!client.connect(mqtt_client_name.c_str(),
                          mqtt_user.c_str(),
                          mqtt_password.c_str(), false))
      {
        Serial.printf("Failure to connect as client: %s\r\n", mqtt_client_name.c_str());
      }

      // Setup subscriptions
      if (client.connected())
      {
        std::stringstream tmp_topic;
        tmp_topic << topic << conf::mqtt::response_topic;
        response_topic.assign(tmp_topic.str());

        if (!client.subscribe(response_topic.c_str()))
        {
          Serial.printf("MQTT Client: failure to subscribe to reply topic %s\r\n", response_topic.c_str());
        }
        else
        {
          online = true;
        }
      }
      else
      {
        Serial.printf("Failure to connect to MQTT server %s\r\n", server_ip.c_str());
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
            Serial.printf("Failed to parse json: %s (%s)", payload, error.c_str());
            throw std::runtime_error("Failed to parse json");
          }
          return RespT::fromJson(doc);
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
    return processQuery<UserResponse, UserQuery>(uid);
  }

  /// @brief Checks the machine status on the server
  /// @param mid machine id
  /// @return server response (if request_ok)
  std::unique_ptr<MachineResponse> FabServer::checkMachine()
  {
    return processQuery<MachineResponse, MachineQuery>();
  }

  /// @brief register the starting of a machine usage
  /// @param uid card uid
  /// @param mid machine id
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::startUse(card::uid_t uid)
  {
    return processQuery<SimpleResponse, StartUseQuery>(uid);
  }

  /// @brief Register end of machine usage
  /// @param uid card ID of the machine user
  /// @param mid machine used ID
  /// @param duration_s duration of usage in seconds
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::finishUse(card::uid_t uid, std::chrono::seconds duration_s)
  {
    return processQuery<SimpleResponse, StopUseQuery>(uid, duration_s);
  }

  /// @brief Registers a maintenance action
  /// @param maintainer who performed the maintenance
  /// @param mid machine maintenance done
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::registerMaintenance(card::uid_t maintainer)
  {
    return processQuery<SimpleResponse, RegisterMaintenanceQuery>(maintainer);
  }

  /// @brief Sends a ping to the server
  /// @param mid machine ID
  /// @return server response (if request_ok)
  std::unique_ptr<SimpleResponse> FabServer::alive()
  {
    return processQuery<SimpleResponse, AliveQuery>();
  }
} // namespace fablabbg