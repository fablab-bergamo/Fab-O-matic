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
  bool maintenanceNeeded; // If true, machine needs maintenance
  bool allowed; // If false, nobody can use the machine

private:
  const Config config;
  bool active;
  FabUser current_user;
  void power(bool on_or_off) const;
  uint32_t usage_start_timestamp;
};

#endif // _MACHINE_H_