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
      POWERED_ON,
      WAITING_FOR_POWER_OFF,
      POWERED_OFF,
      UNKNOWN
    };

    Machine();
    ~Machine() = default;
    Machine(const Machine &) = delete;             // copy constructor
    Machine &operator=(const Machine &x) = delete; // copy assignment
    Machine(Machine &&) = delete;                  // move constructor
    Machine &operator=(Machine &&) = delete;       // move assignment

    bool maintenanceNeeded; // If true, machine needs maintenance
    bool allowed;           // If false, nobody can use the machine

    FabUser getActiveUser() const;
    void configure(const MachineConfig &new_config, FabServer &serv); // Must be called before using the machine
    MachineID getMachineId() const;
    std::string getMachineName() const;
    seconds getUsageDuration() const;
    seconds getAutologoffDelay() const;

    bool login(FabUser user);                       // if the machine is not active, login the user
    void logout();                                  // if the machine is active, check if the card belongs to the user that is logged in and logout the user
    void setAutologoffDelay(seconds new_delay);     // Sets the delay after which the user will be logged off automatically
    void power(bool on_or_off);                     // Power-on or off the machine
    void setMachineName(std::string_view new_name); // Sets the machine name as per backend configuration
    void setMachineType(MachineType new_type);      // Sets the machine type as per backend configuration

    PowerState getPowerState() const; // Gets the current state of the machine
    bool isShutdownImminent() const;  // True if the machine will power down in less than BEEP_REMAINING_MINUTES
    bool isFree() const;              // True is the machine is not used by anybody
    bool canPowerOff() const;         // True if POWEROFF_DELAY_MINUTES delay has expired,and the machine is still idle
    std::string toString() const;
    bool isAutologoffExpired() const; // True if the user shall be logged off automatically
    bool isConfigured() const;        // True if the machine has been configured

    std::optional<MachineConfig> getConfig() const; // Returns the current configuration of the machine

  private:
    std::optional<MachineConfig> config;
    std::optional<std::reference_wrapper<FabServer>> server;

    bool active;
    FabUser current_user;

    std::optional<time_point<system_clock>> usage_start_timestamp; // When did the machine start?
    std::optional<time_point<system_clock>> logoff_timestamp;      // When did the last user log off?
    PowerState power_state;

    void power_mqtt(bool on_or_off);
    void power_relay(bool on_or_off);
  };
} // namespace fablabbg
#endif // MACHINE_H_