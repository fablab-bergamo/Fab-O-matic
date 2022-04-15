#include <cstdint>
#include <array>

#include "MemberClass.h"
#ifndef _MACHINE_H_
#define _MACHINE_H_

typedef uint32_t MachineIDType;
typedef enum enum_MachineType
{
  INVALID     = 0x0,
  PRINTER3D   = 0x1,
  LASER       = 0x2,
  CNC         = 0x3,
  EXTRA1      = 0xFE,
  EXTRA2      = 0xFF
} MachineType;

class MachineClass
{
private:
  const MachineIDType _machine_id;
  const MachineType _machine_type;
  const uint8_t _control_pin;
  const bool _control_pin_active_low;
  
  bool _active;
  MemberClass _current_user;
  void _turnOn();
  void _turnOff();
  unsigned long _usage_start_timestamp;

public:
  MachineClass(MachineIDType machine_id, MachineType machine_type, uint8_t control_pin, bool control_pin_active_low); 
  ~MachineClass();
  bool isFree();
  void begin();
  MemberClass getActiveUser();
  bool isCardWhitelisted(MemberClass user);
  bool login(MemberClass user);  // if the machine is not active, login the user
  void logout(); // if the machine is active, check if the card belongs to the user that is logged in and logout the user
  unsigned long getUsageTime();
};

#endif // _MACHINE_H_