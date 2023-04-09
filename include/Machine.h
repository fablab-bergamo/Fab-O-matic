#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <cstdint>
#include <array>

#include "FabMember.h"

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
    MachineID machine_id {0};
    MachineType machine_type {MachineType::INVALID};
    uint8_t control_pin{0};
    bool control_pin_active_low {false};
    Config(MachineID id, MachineType type, uint8_t pin, bool act_low): machine_id(id), machine_type(type), control_pin(pin), control_pin_active_low(act_low) {}
  };
  Machine(Config config);
  ~Machine() = default;
  bool isFree();
  void begin();
  FabMember getActiveUser();
  bool isCardWhitelisted(FabMember user);
  bool login(FabMember user); // if the machine is not active, login the user
  void logout();              // if the machine is active, check if the card belongs to the user that is logged in and logout the user
  unsigned long getUsageTime();
  Machine::MachineID getMachineId();
  bool operator==(const Machine &v) const;
  bool operator!=(const Machine& v) const;
private:
  const Config config;
  bool active;
  FabMember current_user;
  void power(bool on_or_off);
  uint32_t usage_start_timestamp;
};

#endif // _MACHINE_H_