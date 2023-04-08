#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <cstdint>
#include <array>

#include "FabMember.h"

enum class MachineType
{
  INVALID,
  PRINTER3D,
  LASER,
  CNC,
  EXTRA1,
  EXTRA2
};

struct MachineConfig
{
  uint16_t machine_id;
  MachineType machine_type;
  uint8_t control_pin;
  bool control_pin_active_low;
public:
  MachineConfig() : machine_id(0), machine_type(MachineType::INVALID), control_pin(0), control_pin_active_low(false) {};
  MachineConfig(uint16_t id, MachineType type, uint8_t pin, bool active_low): machine_id(id), machine_type(type), control_pin(pin), control_pin_active_low(active_low) {}
};

class Machine
{
private:
  MachineConfig config;
  bool active;
  FabMember current_user;
  void power(bool on_or_off);
  uint32_t usage_start_timestamp;

public:
  Machine(MachineConfig config);
  ~Machine() = default;
  bool isFree();
  void begin();
  FabMember getActiveUser();
  bool isCardWhitelisted(FabMember user);
  bool login(FabMember user); // if the machine is not active, login the user
  void logout();              // if the machine is active, check if the card belongs to the user that is logged in and logout the user
  unsigned long getUsageTime();
  uint16_t getMachineId();
  bool operator==(const Machine &v) const;
  bool operator!=(const Machine& v) const;
};

#endif // _MACHINE_H_