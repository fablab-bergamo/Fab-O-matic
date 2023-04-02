#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <cstdint>
#include <array>

#include "FabMember.h"

class Machine
{
public:
  typedef uint32_t MachineIDType;
  typedef enum enum_MachineType
  {
    INVALID = 0x0,
    PRINTER3D = 0x1,
    LASER = 0x2,
    CNC = 0x3,
    EXTRA1 = 0xFE,
    EXTRA2 = 0xFF
  } MachineType;

private:
  MachineIDType _machine_id = INVALID;
  MachineType _machine_type = INVALID;
  uint8_t _control_pin = -1;
  bool _control_pin_active_low = false;

  bool _active;
  FabMember _current_user;
  void _turnOn();
  void _turnOff();
  uint32_t _usage_start_timestamp;

public:
  Machine(MachineIDType machine_id, MachineType machine_type, uint8_t control_pin, bool control_pin_active_low);
  ~Machine() = default;
  bool isFree();
  void begin();
  FabMember getActiveUser();
  bool isCardWhitelisted(FabMember user);
  bool login(FabMember user); // if the machine is not active, login the user
  void logout();              // if the machine is active, check if the card belongs to the user that is logged in and logout the user
  unsigned long getUsageTime();
  MachineIDType getMachineId();
  bool operator==(const Machine &v) const;
  bool operator!=(const Machine& v) const;
};

#endif // _MACHINE_H_