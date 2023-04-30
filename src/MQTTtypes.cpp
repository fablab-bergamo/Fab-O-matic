#include "MQTTtypes.h"
#include "card.h"
#include "string"
#include <memory>
#include "FabUser.h"
#include <string_view>
#include <sstream>
#include "ArduinoJson.h"

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
    ss << "{\"action\":\"checkmachine\","
       << "\"mid\":\"" << this->mid.id << "\""
       << "}";
    return ss.str();
  }

  std::string AliveQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"alive\","
       << "\"mid\":\"" << this->mid.id << "\""
       << "}";
    return ss.str();
  }

  std::string StartUseQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"startuse\", "
       << "\"uid\":\"" << card::uid_str(this->uid) << "\","
       << "\"mid\":" << this->mid.id
       << "}";
    return ss.str();
  }

  std::string StopUseQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"stopuse\", "
       << "\"uid\":\"" << card::uid_str(this->uid) << "\","
       << "\"mid\":" << this->mid.id << ","
       << "\"duration\":" << this->duration_s.count()
       << "}";
    return ss.str();
  }

  std::string RegisterMaintenanceQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"maintenance\", "
       << "\"uid\":\"" << card::uid_str(this->uid) << "\","
       << "\"mid\":" << this->mid.id
       << "}";
    return ss.str();
  }

  std::unique_ptr<UserResponse> UserResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<UserResponse>(false);
    response->request_ok = doc["request_ok"];
    response->is_valid = doc["is_valid"];
    response->holder_name = doc["name"].as<std::string>();
    response->user_level = static_cast<FabUser::UserLevel>(doc["level"].as<int>());

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Parsed user response as request_ok %d is_valid %d name %s level %d\r\n", response->request_ok, response->is_valid, response->holder_name.data(), response->user_level);

    return response;
  }

  std::unique_ptr<MachineResponse> MachineResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<MachineResponse>(false);
    response->request_ok = doc["request_ok"];
    response->is_valid = doc["is_valid"];
    response->needs_maintenance = doc["maintenance"];
    response->allowed = doc["allowed"];

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Parsed machine response as request_ok %d is_valid %d maintenance %d allowed %d\r\n", response->request_ok, response->is_valid, response->needs_maintenance, response->allowed);

    return response;
  }

  std::unique_ptr<SimpleResponse> SimpleResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<SimpleResponse>(false);
    response->request_ok = doc["request_ok"];

    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Parsed simple response as request_ok %d\r\n", response->request_ok);

    return response;
  }
} // namespace ServerMQTT