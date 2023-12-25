#include "MQTTtypes.hpp"
#include "card.hpp"
#include "string"
#include <memory>
#include "FabUser.hpp"
#include <string_view>
#include <sstream>
#include "ArduinoJson.hpp"

namespace ServerMQTT
{
  std::string UserQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"checkuser\","
       << "\"uid\":\"" << card::uid_str(this->uid) << "\""
       << "}";
    return ss.str();
  }

  std::string MachineQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"checkmachine\""
       << "}";
    return ss.str();
  }

  std::string AliveQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"alive\""
       << "}";
    return ss.str();
  }

  std::string StartUseQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"startuse\", "
       << "\"uid\":\"" << card::uid_str(this->uid) << "\""
       << "}";
    return ss.str();
  }

  std::string StopUseQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"stopuse\", "
       << "\"uid\":\"" << card::uid_str(this->uid) << "\","
       << "\"duration\":" << this->duration_s.count()
       << "}";
    return ss.str();
  }

  std::string RegisterMaintenanceQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"maintenance\", "
       << "\"uid\":\"" << card::uid_str(this->uid) << "\""
       << "}";
    return ss.str();
  }

  UserResult UserResponse::getResult() const
  {
    return static_cast<UserResult>(this->result);
  }
  std::unique_ptr<UserResponse> UserResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<UserResponse>(doc["request_ok"].as<bool>());
    response->result = doc["is_valid"];
    response->holder_name = doc["name"].as<std::string>();
    response->user_level = static_cast<FabUser::UserLevel>(doc["level"].as<int>());

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Parsed user response as request_ok %d result %u name %s level %u\r\n", response->request_ok, static_cast<uint8_t>(response->result), response->holder_name.data(), static_cast<uint8_t>(response->user_level));

    return response;
  }

  std::unique_ptr<MachineResponse> MachineResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<MachineResponse>(doc["request_ok"].as<bool>());
    response->is_valid = doc["is_valid"];
    response->needs_maintenance = doc["maintenance"];
    response->allowed = doc["allowed"];
    response->timeout_min = doc["timeout_min"];
    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Parsed machine response as request_ok %d is_valid %d maintenance %d allowed %d autologoff %d\r\n",
                    response->request_ok, response->is_valid, response->needs_maintenance, response->allowed, response->timeout_min);

    return response;
  }

  std::unique_ptr<SimpleResponse> SimpleResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<SimpleResponse>(doc["request_ok"].as<bool>());

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Parsed simple response as request_ok %d\r\n", response->request_ok);

    return response;
  }
} // namespace ServerMQTT