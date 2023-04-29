#ifndef MQTTTYPES_H_
#define MQTTTYPES_H_

#include "card.h"
#include "string"
#include <memory>
#include "FabUser.h"
#include "Machine.h"
#include <string_view>
#include "ArduinoJson.h"

namespace ServerMQTT
{

  class Query
  {
  public:
    virtual std::string payload() const = 0;
  };

  class UserQuery : public Query
  {
  public:
    const card::uid_t uid;
    UserQuery(card::uid_t card_uid) : uid(card_uid){};
    std::string payload() const;
  };

  class MachineQuery : public Query
  {
  public:
    const Machine::MachineID mid;
    MachineQuery(Machine::MachineID mid) : mid(mid){};
    std::string payload() const;
  };

  class AliveQuery : public Query
  {
  public:
    const Machine::MachineID mid;
    AliveQuery(Machine::MachineID mid) : mid(mid){};
    std::string payload() const;
  };

  class StartUseQuery : public Query
  {
  public:
    const card::uid_t uid;
    const Machine::MachineID mid;
    StartUseQuery(card::uid_t card_uid, Machine::MachineID mid) : uid(card_uid), mid(mid){};
    std::string payload() const;
  };

  class StopUseQuery : public Query
  {
  public:
    const card::uid_t uid;
    const Machine::MachineID mid;
    const uint16_t duration_s;
    /// @brief Request to register machine usage stop
    /// @param card_uid machine user card id
    /// @param mid machine id
    /// @param duration duration of usage, in seconds
    StopUseQuery(card::uid_t card_uid, Machine::MachineID mid, uint16_t duration) : uid(card_uid), mid(mid), duration_s(duration){};
    std::string payload() const;
  };

  class RegisterMaintenanceQuery : public Query
  {
  public:
    const card::uid_t uid;
    const Machine::MachineID mid;
    RegisterMaintenanceQuery(card::uid_t card_uid, Machine::MachineID mid) : uid(card_uid), mid(mid){};
    std::string payload() const;
  };

  class Response
  {
  protected:
    static void loadJson(JsonDocument &doc);
  };

  class UserResponse : public Response
  {
  public:
    bool request_ok = false;       /* True if the request was processed by the server*/
    bool is_valid = false;         /* True if the user is valid */
    std::string holder_name;       /* Name of the user from server DB */
    FabUser::UserLevel user_level; /* User priviledges */
    UserResponse(bool rok) : request_ok(rok){};
    static std::unique_ptr<UserResponse> fromJson(JsonDocument &doc);
  };

  class MachineResponse : public Response
  {
  public:
    bool request_ok = false;       /* True if the request was processed by the server */
    bool is_valid = false;         /* True if the machine has a valid ID */
    bool needs_maintenance = true; /* True if the machine needs maintenance */
    bool allowed = false;          /* True if the machine can be used by anybody */
    MachineResponse(bool rok) : request_ok(rok){};
    static std::unique_ptr<MachineResponse> fromJson(JsonDocument &doc);
  };

  class SimpleResponse : public Response
  {
  public:
    bool request_ok = false; /* True if the request was processed by the server */
    SimpleResponse(bool result) : request_ok(result){};
    static std::unique_ptr<SimpleResponse> fromJson(JsonDocument &doc);
  };

} // namespace ServerMQTT
#endif
