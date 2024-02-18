#include "MQTTtypes.hpp"
#include "ArduinoJson.hpp"
#include "FabUser.hpp"
#include "card.hpp"
#include "string"
#include <memory>
#include <sstream>
#include <string_view>

#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif

namespace fablabbg::ServerMQTT
{
  std::string UserQuery::payload() const
  {
    std::stringstream ss{};
    ss << "{\"action\":\"checkuser\","
       << "\"uid\":\"" << card::uid_str(uid) << "\""
       << "}";
    return ss.str();
  }

  std::string MachineQuery::payload() const
  {
    std::stringstream ss{};
    ss << "{\"action\":\"checkmachine\""
       << "}";
    return ss.str();
  }

  std::string AliveQuery::payload() const
  {
    std::stringstream ss{};
    ss << "{\"action\":\"alive\","
       << "version\":\"" << GIT_VERSION << "\""
       << "}";
    return ss.str();
  }

  std::string StartUseQuery::payload() const
  {
    std::stringstream ss{};
    ss << "{\"action\":\"startuse\", "
       << "\"uid\":\"" << card::uid_str(uid) << "\""
       << "}";
    return ss.str();
  }

  std::string StopUseQuery::payload() const
  {
    std::stringstream ss{};
    ss << "{\"action\":\"stopuse\", "
       << "\"uid\":\"" << card::uid_str(uid) << "\","
       << "\"duration\":" << duration_s.count()
       << "}";
    return ss.str();
  }

  std::string InUseQuery::payload() const
  {
    std::stringstream ss{};
    ss << "{\"action\":\"inuse\", "
       << "\"uid\":\"" << card::uid_str(uid) << "\","
       << "\"duration\":" << duration_s.count()
       << "}";
    return ss.str();
  }

  std::string RegisterMaintenanceQuery::payload() const
  {
    std::stringstream ss{};
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

  std::string UserResponse::toString() const
  {
    std::stringstream ss{};
    ss << "UserResponse: "
       << "request_ok: " << request_ok << ", "
       << "result: " << static_cast<int>(result) << ", "
       << "holder_name: " << holder_name << ", "
       << "user_level: " << static_cast<int>(user_level);
    return ss.str();
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
    if (!doc["grace"].isNull())
    {
      response->grace = doc["grace"];
    }
    else
    {
      response->grace = std::chrono::duration_cast<std::chrono::minutes>(conf::machine::DEFAULT_GRACE_PERIOD).count();
    }

    return response;
  }

  std::unique_ptr<SimpleResponse> SimpleResponse::fromJson(JsonDocument &doc)
  {
    auto response = std::make_unique<SimpleResponse>(doc["request_ok"].as<bool>());
    return response;
  }
} // namespace fablabbg::ServerMQTT