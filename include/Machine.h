#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <cstdint>
#include <array>

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
    uint16_t id;
  };
  struct Config
  {
    MachineID machine_id{0};
    MachineType machine_type{MachineType::INVALID};
    std::string machine_name;
    uint8_t control_pin{0};
    bool control_pin_active_low{false};
    Config(MachineID id, MachineType type, std::string name, uint8_t pin, bool act_low) : machine_id(id), machine_type(type), machine_name(name), control_pin(pin), control_pin_active_low(act_low) {}
  };
  Machine(Config config);
  ~Machine() = default;
  bool isFree() const;
  FabUser &getActiveUser();
  bool login(FabUser user); // if the machine is not active, login the user
  void logout();            // if the machine is active, check if the card belongs to the user that is logged in and logout the user
  unsigned long getUsageTime() const;
  Machine::MachineID getMachineId() const;
  std::string getMachineName() const;
  bool operator==(const Machine &v) const;
  bool operator!=(const Machine &v) const;
  bool maintenanceNeeded;             // If true, machine needs maintenance
  bool allowed;                       // If false, nobody can use the machine
  bool canPowerOff() const;           // True if POWEROFF_DELAY_MINUTES delay has expired,and the machine is still idle
  void power(bool on_or_off);         // Power-on or off the machine
  PowerState getPowerState() const;   // Gets the current state of the machine
  bool shutdownWarning() const;       // True if the machine will power down in less than BEEP_REMAINING_MINUTES

private:
  const Config config;
  bool active;
  FabUser current_user;
  uint32_t usage_start_timestamp;     // When did the machine start
  uint32_t power_off_min_timestamp;   // Minimum allowed timestamp for power-down
  PowerState powerState;

};

#endif // _MACHINE_H_