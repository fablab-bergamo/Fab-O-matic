#include "Machine.h"
#include "Arduino.h"

#include <cstdint>

Machine::Machine(
    MachineIDType machine_id,
    MachineType machine_type,
    uint8_t control_pin,
    bool control_pin_active_low) : _machine_id(machine_id),
                                   _machine_type(machine_type),
                                   _control_pin(control_pin),
                                   _control_pin_active_low(control_pin_active_low)
{
  _active = false;
  _usage_start_timestamp = 0;
  _current_user = FabMember();
  pinMode(_control_pin, OUTPUT);
  digitalWrite(_control_pin, _control_pin_active_low ? HIGH : LOW);
}

Machine::MachineIDType Machine::getMachineId()
{
  return _machine_id;
}

bool Machine::isFree()
{
  return !_active;
}

bool Machine::login(FabMember user)
{
  if (isFree())
  {
    this->_active = true;
    this->_current_user = user;
    this->_turnOn();
    this->_usage_start_timestamp = millis();
    return true;
  }
  return false;
}

void Machine::logout()
{
  if (_active)
  {
    _active = false;
    this->_turnOff();
  }
}

void Machine::_turnOn()
{
  if (_control_pin_active_low)
  {
    digitalWrite(_control_pin, LOW);
  }
  else
  {
    digitalWrite(_control_pin, HIGH);
  }
}

void Machine::_turnOff()
{
  if (_control_pin_active_low)
  {
    digitalWrite(_control_pin, HIGH);
  }
  else
  {
    digitalWrite(_control_pin, LOW);
  }
}

FabMember Machine::getActiveUser()
{
  return _current_user;
}

unsigned long Machine::getUsageTime()
{
  if (_active)
  {
    return millis() - _usage_start_timestamp;
  }
  return 0;
}

bool Machine::operator==(const Machine &v) const
{
  return (_machine_id == v._machine_id);
}

bool Machine::operator!=(const Machine &v) const
{
  return (_machine_id != v._machine_id);
}