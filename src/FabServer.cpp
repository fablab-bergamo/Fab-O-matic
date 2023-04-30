#include "FabServer.h"
#include "secrets.h"

#include <string>
#include <string_view>
#include <cstdint>
#include <sstream>
#include <ArduinoJson.h>

using namespace ServerMQTT;

/// @brief FabServer API interface class
/// @param ssid wifi network
/// @param password wifi password
/// @param server_ip server IP address
FabServer::FabServer(std::string_view ssid, std::string_view password, std::string_view server_ip) : wifi_ssid(ssid), wifi_password(password), server_ip(server_ip), online(false)
{
  std::stringstream ss;
  ss << secrets::mqtt::topic << "/" << secrets::machine::machine_id.id;
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
        Serial.printf("Received answer: %s\r\n", this->last_reply.data());
      return true;
    }
  }
  return false;
}

/// @brief posts to MQTT server
/// @param query message to post
/// @return true if the message was published
bool FabServer::publish(const Query &query)
{

  String payload(query.payload().data());
  String topic(this->topic.c_str());

  if (payload.length() > FabServer::MAX_MQTT_LENGTH)
  {
    Serial.printf("Message is too long: %s\r\n", payload);
    return false;
  }

  this->answer_pending = true;
  this->last_query = payload.c_str();

  if (conf::debug::ENABLE_LOGS)
    Serial.printf("Sending MQTT message %s on topic %s\r\n", payload.c_str(), topic.c_str());

  return this->client.publish(topic, payload);
}

bool FabServer::loop()
{
  if (!this->client.loop())
  {
    if (this->online && conf::debug::ENABLE_LOGS)
    {
      Serial.println("MQTT connection lost");
    }
    this->online = false;
    return false;
  }
  return true;
}

/// @brief Returns a fake server reply for testing purposes
/// @return json payload
String FabServer::fakeReply() const
{
  if (this->last_query.find("checkmachine") != std::string::npos)
  {
    String payload = "{\"request_ok\":true,\"is_valid\":true,\"allowed\":true,\"maintenance\":false}";
    return payload;
  }

  if (this->last_query.find("maintenance") != std::string::npos)
  {
    String payload = "{\"request_ok\":true}";
    return payload;
  }

  if (this->last_query.find("startuse") != std::string::npos)
  {
    String payload = "{\"request_ok\":true}";
    return payload;
  }

  if (this->last_query.find("stopuse") != std::string::npos)
  {
    String payload = "{\"request_ok\":true}";
    return payload;
  }

  if (this->last_query.find("checkuser") != std::string::npos)
  {
    String payload = "{\"request_ok\":true,\"level\":2,\"name\":\"FAKE USER\",\"is_valid\":true}";
    return payload;
  }

  String payload = "{\"request_ok\":true}";
  return payload;
}

/// @brief blocks until the server answers or until the timeout is reached
/// @return true if the server answered
bool FabServer::waitForAnswer()
{
  constexpr uint16_t MAX_DURATION_MS = 2000;
  constexpr uint16_t DELAY_MS = 100;
  constexpr uint8_t NB_TRIES = (MAX_DURATION_MS / DELAY_MS);

  for (auto i = 0; i < NB_TRIES; i++)
  {
    if (this->answer_pending)
    {
      this->client.loop();
      delay(DELAY_MS);
      if (conf::debug::FAKE_BACKEND)
      {
        String topic = this->response_topic.c_str();
        String payload = this->fakeReply();
        this->messageReceived(topic, payload);
      }
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
void FabServer::messageReceived(String &topic, String &payload)
{
  std::stringstream ss;
  ss << "Received message, topic:" << topic.c_str() << ", payload:" << payload.c_str();
  Serial.println(ss.str().c_str());
  this->last_reply = payload.c_str();
  this->answer_pending = false;
}

/// @brief Establish WiFi connection and connects to FabServer
/// @return true if both operations succeeded
bool FabServer::connect()
{
  constexpr uint8_t NB_TRIES = 3;
  constexpr uint16_t DELAY_MS = 1000;

  // Connect WiFi if needed
  if (this->WiFiConnection.status() != WL_CONNECTED)
  {
    this->WiFiConnection.begin(this->wifi_ssid.data(), this->wifi_password.data());
    for (auto i = 0; i < NB_TRIES; i++)
    {
      if (conf::debug::ENABLE_LOGS && this->WiFiConnection.status() == WL_CONNECTED)
        Serial.println("WiFi connection successfull");
      break;
      delay(DELAY_MS);
    }
  }

  // Check server
  if (this->WiFiConnection.status() == WL_CONNECTED)
  {
    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Connected to WiFi %s\r\n", this->wifi_ssid.data());

    this->client.begin(secrets::mqtt::server.data(), this->net);

    this->callback = [&](String &a, String &b)
    { return this->messageReceived(a, b); };

    this->client.onMessage(this->callback);

    if (!client.connect("ESP32", secrets::mqtt::user.data(), secrets::mqtt::password.data(), false))
    {
      Serial.printf("Failure to connect as client: %s\r\n", secrets::mqtt::client.data());
    }

    if (client.connected())
    {
      this->online = true;
      std::stringstream tmp_topic;
      tmp_topic << this->topic << secrets::mqtt::response_topic;
      this->response_topic = tmp_topic.str();

      if (!client.subscribe(this->response_topic.c_str()))
      {
        Serial.printf("Failure to subscribe to reply topic %s\r\n", this->response_topic.c_str());
        this->online = false;
      }
      // TODO ??
    }
  }
  else
  {
    Serial.printf("Failure to connect to WiFi %s\r\n", this->wifi_ssid.data());
    this->online = false;
  }

  if (conf::debug::ENABLE_LOGS)
  {
    std::stringstream ss;
    ss << "Online:" << this->online << ", board IP address:" << WiFi.localIP() << ", server: " << secrets::mqtt::server;
    Serial.println(ss.str().c_str());
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
        const auto payload = this->last_reply.data();
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
std::unique_ptr<MachineResponse> FabServer::checkMachine(Machine::MachineID mid)
{
  return this->processQuery<MachineResponse, MachineQuery>(mid);
}

/// @brief register the starting of a machine usage
/// @param uid card uid
/// @param mid machine id
/// @return server response (if request_ok)
std::unique_ptr<SimpleResponse> FabServer::startUse(card::uid_t uid, Machine::MachineID mid)
{
  return this->processQuery<SimpleResponse, StartUseQuery>(uid, mid);
}

/// @brief Register end of machine usage
/// @param uid card ID of the machine user
/// @param mid machine used ID
/// @param duration_s duration of usage in seconds
/// @return server response (if request_ok)
std::unique_ptr<SimpleResponse> FabServer::finishUse(card::uid_t uid, Machine::MachineID mid, uint16_t duration_s)
{
  return this->processQuery<SimpleResponse, StopUseQuery>(uid, mid, duration_s);
}

/// @brief Registers a maintenance action
/// @param maintainer who performed the maintenance
/// @param mid machine maintenance done
/// @return server response (if request_ok)
std::unique_ptr<SimpleResponse> FabServer::registerMaintenance(card::uid_t maintainer, Machine::MachineID mid)
{
  return this->processQuery<SimpleResponse, RegisterMaintenanceQuery>(maintainer, mid);
}

/// @brief Sends a ping to the server
/// @param mid machine ID
/// @return server response (if request_ok)
std::unique_ptr<SimpleResponse> FabServer::alive(const Machine::MachineID mid)
{
  return this->processQuery<SimpleResponse, AliveQuery>(mid);
}