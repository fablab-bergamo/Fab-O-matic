#ifndef MACHINE_H_
#define MACHINE_H_

#include <cstdint>
#include <array>
#include <chrono>

#include "FabUser.h"

class Machine
{
public:
  enum class MachineType
  {
    INVALID,
    PRINTER3D,
    LASER,
    CNC,
    EXTRA1,
    EXTRA2
  };
  enum class PowerState
  {
    POWERED_ON,
    WAITING_FOR_POWER_OFF,
    POWERED_OFF
  };
  struct MachineID
  {
    const uint16_t id;
  };
  struct Config
  {
    const MachineID machine_id{0};
    const MachineType machine_type{MachineType::INVALID};
    const std::string machine_name;
    const uint8_t control_pin{0};
    const bool control_pin_active_low{false};
    Config(MachineID id, MachineType type, std::string_view name, uint8_t pin, bool act_low) : machine_id(id), machine_type(type), machine_name(name), control_pin(pin), control_pin_active_low(act_low) {}
  };

  Machine(const Config config);
  ~Machine() = default;

  bool maintenanceNeeded; // If true, machine needs maintenance
  bool allowed;           // If false, nobody can use the machine

  FabUser &getActiveUser();
  Machine::MachineID getMachineId() const;
  std::string getMachineName() const;
  std::chrono::seconds getUsageDuration() const;
  PowerState getPowerState() const; // Gets the current state of the machine
  bool isShutdownImminent() const;  // True if the machine will power down in less than BEEP_REMAINING_MINUTES
  bool isFree() const;

  bool login(FabUser user);         // if the machine is not active, login the user
  void logout();                    // if the machine is active, check if the card belongs to the user that is logged in and logout the user
  bool canPowerOff() const;         // True if POWEROFF_DELAY_MINUTES delay has expired,and the machine is still idle
  void power(const bool on_or_off); // Power-on or off the machine

  bool operator==(const Machine &v) const;
  bool operator!=(const Machine &v) const;
  std::string toString() const;

private:
  const Config config;
  bool active;
  FabUser current_user;
  std::optional<time_point<system_clock>> usage_start_timestamp; // When did the machine start
  std::optional<time_point<system_clock>> logout_timestamp;      // Minimum allowed timestamp for power-down
  PowerState powerState;
};

#endif // MACHINE_H_