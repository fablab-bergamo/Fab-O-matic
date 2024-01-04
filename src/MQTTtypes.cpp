#include "MQTTtypes.hpp"
#include "card.hpp"
#include "string"
#include <memory>
#include "FabUser.hpp"
#include <string_view>
#include <sstream>
#include "ArduinoJson.hpp"

namespace fablabbg::ServerMQTT
{
  std::string UserQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"checkuser\","
       << "\"uid\":\"" << card::uid_str(uid) << "\""
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
       << "\"uid\":\"" << card::uid_str(uid) << "\""
       << "}";
    return ss.str();
  }

  std::string StopUseQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"stopuse\", "
       << "\"uid\":\"" << card::uid_str(uid) << "\","
       << "\"duration\":" << duration_s.count()
       << "}";
    return ss.str();
  }

  std::string RegisterMaintenanceQuery::payload() const
  {
    std::stringstream ss;
    ss << "{\"action\":\"maintenance\", "
       << "\"uid\":\"" << card::uid_str(uid) << "\""
       << "}";
    return ss.str();
  }

  UserResult UserResponse::getResult() const
  {
    return static_cast<UserResult>(result);
  }
  std::unique_ptr<UserResponse> UserResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<UserResponse>(doc["request_ok"].as<bool>());
    response->result = doc["is_valid"];
    response->holder_name = doc["name"].as<std::string>();
    response->user_level = static_cast<FabUser::UserLevel>(doc["level"].as<int>());

    return response;
  }

  std::unique_ptr<MachineResponse> MachineResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<MachineResponse>(doc["request_ok"].as<bool>());
    response->is_valid = doc["is_valid"];
    response->maintenance = doc["maintenance"];
    response->allowed = doc["allowed"];
    response->logoff = doc["logoff"];
    response->name = doc["name"].as<std::string>();
    response->type = doc["type"];

    return response;
  }

  std::unique_ptr<SimpleResponse> SimpleResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<SimpleResponse>(doc["request_ok"].as<bool>());
    return response;
  }
} // namespace ServerMQTT