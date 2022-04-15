#include "MachineClass.h"
#include "Arduino.h"

#include <cstdint>



MachineClass::MachineClass(
  MachineIDType machine_id, 
  MachineType machine_type, 
  uint8_t control_pin,
  bool control_pin_active_low
) :
  _machine_id(machine_id),
  _machine_type(machine_type),
  _control_pin(control_pin),
  _control_pin_active_low(control_pin_active_low)
{
  _active = false;
  _usage_start_timestamp = 0;
  _current_user = MemberClass();
  pinMode(_control_pin, OUTPUT);
  digitalWrite(_control_pin, _control_pin_active_low ? HIGH : LOW);  
}


MachineClass::~MachineClass()
{
}

bool MachineClass::isFree()
{
  return !_active;
}

bool MachineClass::login(MemberClass user)
{
  if(isFree()){
    this->_active = true;
    this->_current_user = user;
    this->_turnOn();
    this->_usage_start_timestamp = millis();
    return true;
  }
  return false;
}

void MachineClass::logout()
{
  if(_active){
    _active = false;
    this->_turnOff();
  }
}

void MachineClass::_turnOn()
{
  if(_control_pin_active_low){
    digitalWrite(_control_pin, LOW);
  }
  else{
    digitalWrite(_control_pin, HIGH);
  }
}

void MachineClass::_turnOff()
{
  if(_control_pin_active_low){
    digitalWrite(_control_pin, HIGH);
  }
  else{
    digitalWrite(_control_pin, LOW);
  }
}

MemberClass MachineClass::getActiveUser()
{
  return _current_user;
}

unsigned long MachineClass::getUsageTime()
{
  if(_active){
    return millis() - _usage_start_timestamp;
  }
  return 0;
}