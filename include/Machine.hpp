#ifndef MACHINE_H_
#define MACHINE_H_

#include <cstdint>
#include <array>
#include <chrono>
#include "FabUser.hpp"
#include "MachineConfig.hpp"

namespace fablabbg
{
  class FabServer;

  class Machine
  {
  public:
    enum class PowerState
    {
      UNKNOWN,
      POWERED_ON,
      WAITING_FOR_POWER_OFF,
      POWERED_OFF
    };

    constexpr Machine(){};

    ~Machine() = default;
    Machine(const Machine &) = delete;             // copy constructor
    Machine &operator=(const Machine &x) = delete; // copy assignment
    Machine(Machine &&) = delete;                  // move constructor
    Machine &operator=(Machine &&) = delete;       // move assignment

    /// @brief If true, machine needs maintenance
    bool maintenanceNeeded{false};
    /// @brief If true, machine is allowed to be used by anybody
    bool allowed{true};

    FabUser getActiveUser() const;

    /// @brief Configure the machine, it must be called before most methods.
    void configure(const MachineConfig &new_config, FabServer &serv);

    MachineID getMachineId() const;
    std::string getMachineName() const;

    /// @brief Duration of the current usage, or 0s
    seconds getUsageDuration() const;

    seconds getAutologoffDelay() const;

    /// @brief Try to login the user and start the usage timer
    bool login(FabUser user);

    /// @brief Logoff the user and stop the usage timer
    void logout();

    /// @brief Sets the delay after which the user will be logged off automatically
    void setAutologoffDelay(seconds new_delay);

    /// @brief Powers the machine on or off using relay/MQTT/both
    /// @param on_or_off new power state
    void power(bool on_or_off);

    /// @brief Sets the machine name as per backend configuration
    /// @param new_name Will be shown on LCD, keep it short.
    void setMachineName(const std::string &new_name);

    /// @brief Sets the machine type as per backend configuration
    void setMachineType(MachineType new_type);

    /// @brief Returns the current power state of the machine
    PowerState getPowerState() const;

    /// @brief Indicates if the machine will power down in less than BEEP_REMAINING_MINUTES
    bool isShutdownImminent() const;

    /// @brief Indicates is the machine is not used by anybody
    bool isFree() const;

    /// @brief Indicates if POWEROFF_DELAY_MINUTES delay has expired,and the machine is still idle
    bool canPowerOff() const;
    std::string toString() const;

    /// @brief Indicates ff the user shall be logged off automatically
    bool isAutologoffExpired() const;

    /// @brief Indicates if the machine has been configured
    bool isConfigured() const;

    /// @brief Returns the current configuration of the machine, used for testing.
    std::optional<MachineConfig> getConfig() const;

  private:
    std::optional<MachineConfig> config{std::nullopt};
    std::optional<std::reference_wrapper<FabServer>> server{std::nullopt};

    bool active{false};
    FabUser current_user{};

    std::optional<time_point<system_clock>> usage_start_timestamp{std::nullopt}; // When did the machine start?
    std::optional<time_point<system_clock>> logoff_timestamp{std::nullopt};      // When did the last user log off?
    PowerState power_state{PowerState::POWERED_OFF};

    void power_mqtt(bool on_or_off);
    void power_relay(bool on_or_off);
  };
} // namespace fablabbg
#endif // MACHINE_H_