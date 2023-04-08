#include "Machine.h"
#include "Arduino.h"

#include <cstdint>

Machine::Machine(MachineConfig user_conf) : config(user_conf), active(false), usage_start_timestamp(0)
{
  this->current_user = FabMember();
  pinMode(this->config.control_pin, OUTPUT);
  digitalWrite(this->config.control_pin, this->config.control_pin_active_low ? HIGH : LOW);
}

u_int16_t Machine::getMachineId()
{
  return this->config.machine_id;
}

bool Machine::isFree()
{
  return !this->active;
}

bool Machine::login(FabMember user)
{
  if (this->isFree())
  {
    this->active = true;
    this->current_user = user;
    this->power(true);
    this->usage_start_timestamp = millis();
    return true;
  }
  return false;
}

void Machine::logout()
{
  if (this->active)
  {
    active = false;
    this->power(false);
  }
}

void Machine::power(bool value)
{
  if (this->config.control_pin_active_low)
  {
    digitalWrite(this->config.control_pin, value ? LOW : HIGH);
  }
  else
  {
    digitalWrite(this->config.control_pin, value ? LOW : HIGH);
  }
}

FabMember Machine::getActiveUser()
{
  return this->current_user;
}

unsigned long Machine::getUsageTime()
{
  if (this->active)
  {
    return millis() - this->usage_start_timestamp;
  }
  return 0;
}

bool Machine::operator==(const Machine &v) const
{
  return (this->config.machine_id == v.config.machine_id);
}

bool Machine::operator!=(const Machine &v) const
{
  return (this->config.machine_id != v.config.machine_id);
}
