#ifndef MQTTTYPES_H_
#define MQTTTYPES_H_

#include "card.hpp"
#include "string"
#include <memory>
#include "FabUser.hpp"
#include "Machine.hpp"
#include <string_view>
#include "ArduinoJson.h"

namespace fablabbg::ServerMQTT
{
  class Query
  {
  public:
    virtual std::string payload() const = 0;
    virtual ~Query() = default;
  };

  class UserQuery final : public Query
  {
  public:
    const card::uid_t uid;

    UserQuery() = delete;
    constexpr UserQuery(card::uid_t card_uid) : uid(card_uid){};

    std::string payload() const;
  };

  class MachineQuery final : public Query
  {
  public:
    constexpr MachineQuery() = default;
    std::string payload() const;
  };

  class AliveQuery final : public Query
  {
  public:
    constexpr AliveQuery() = default;
    std::string payload() const;
  };

  class StartUseQuery final : public Query
  {
  public:
    const card::uid_t uid;

    StartUseQuery() = delete;
    constexpr StartUseQuery(card::uid_t card_uid) : uid(card_uid){};

    std::string payload() const;
  };

  class StopUseQuery final : public Query
  {
  public:
    const card::uid_t uid;
    const std::chrono::seconds duration_s;

    StopUseQuery() = delete;

    /// @brief Request to register machine usage stop
    /// @param card_uid machine user card id
    /// @param mid machine id
    /// @param duration duration of usage, in seconds
    constexpr StopUseQuery(card::uid_t card_uid, std::chrono::seconds duration) : uid(card_uid), duration_s(duration){};
    std::string payload() const;
  };

  class InUseQuery final : public Query
  {
  public:
    const card::uid_t uid;
    const std::chrono::seconds duration_s;

    InUseQuery() = delete;

    /// @brief Request to register machine usage stop
    /// @param card_uid machine user card id
    /// @param mid machine id
    /// @param duration duration of usage, in seconds
    constexpr InUseQuery(card::uid_t card_uid, std::chrono::seconds duration) : uid(card_uid), duration_s(duration){};
    std::string payload() const;
  };

  class RegisterMaintenanceQuery final : public Query
  {
  public:
    const card::uid_t uid;

    RegisterMaintenanceQuery() = delete;
    constexpr RegisterMaintenanceQuery(card::uid_t card_uid) : uid(card_uid){};

    std::string payload() const;
  };

  class Response
  {
  public:
    const bool request_ok{false}; /* True if the request was processed by the server */

    Response() = delete;
    constexpr Response(bool result) : request_ok(result){};

  protected:
    static void loadJson(JsonDocument &doc);
  };

  enum class UserResult : uint8_t
  {
    USER_INVALID = 0,
    USER_AUTHORIZED = 1,
    USER_UNAUTHORIZED = 2,
    USER_UNAUTHORIZED_MAINTENANCE = 3
  };

  class UserResponse final : public Response
  {
  public:
    uint8_t result{static_cast<uint8_t>(UserResult::USER_INVALID)}; /* Result of the user check */
    std::string holder_name{""};                                    /* Name of the user from server DB */
    FabUser::UserLevel user_level{FabUser::UserLevel::UNKNOWN};     /* User priviledges */

    UserResponse() = delete;
    UserResponse(bool rok) : Response(rok){};

    UserResponse(bool rok, UserResult res) : Response(rok),
                                             result(static_cast<uint8_t>(res)){};

    [[nodiscard]] static std::unique_ptr<UserResponse> fromJson(JsonDocument &doc);
    UserResult getResult() const;
    std::string toString() const;
  };

  class MachineResponse final : public Response
  {
  public:
    bool is_valid = false;   /* True if the machine has a valid ID */
    bool maintenance = true; /* True if the machine needs maintenance */
    bool allowed = false;    /* True if the machine can be used by anybody */
    uint16_t logoff = 0;     /* Timeout in minutes */
    std::string name{""};    /* Name of the machine from server DB */
    uint8_t type = 0;        /* Type of the machine */
    MachineResponse() = delete;
    MachineResponse(bool rok) : Response(rok){};

    [[nodiscard]] static std::unique_ptr<MachineResponse> fromJson(JsonDocument &doc);
  };

  class SimpleResponse final : public Response
  {
  public:
    SimpleResponse() = delete;
    constexpr SimpleResponse(bool rok) : Response(rok){};

    [[nodiscard]] static std::unique_ptr<SimpleResponse> fromJson(JsonDocument &doc);
  };

} // namespace fablabbg::ServerMQTT
#endif
