#include "Machine.h"
#include "Arduino.h"

#include <cstdint>

Machine::Machine(Config user_conf) : config(user_conf), active(false), usage_start_timestamp(0), maintenanceNeeded(false), allowed(true)
{
  this->current_user = FabUser();
  pinMode(this->config.control_pin, OUTPUT); 
  this->power(false);
  Serial.printf("Machine %s configured on pin %d (active_low:%d)\n", this->config.machine_name.c_str(), this->config.control_pin, this->config.control_pin_active_low);
}

Machine::MachineID Machine::getMachineId() const
{
  return this->config.machine_id;
}

bool Machine::isFree() const
{
  return !this->active;
}

bool Machine::login(FabUser user)
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

void Machine::power(bool value) const
{
  if (this->config.control_pin_active_low)
  {
    digitalWrite(this->config.control_pin, value ? HIGH : LOW);
  }
  else
  {
    digitalWrite(this->config.control_pin, value ? LOW : HIGH);
  }
}

FabUser& Machine::getActiveUser()
{
  return this->current_user;
}

unsigned long Machine::getUsageTime() const
{
  if (this->active)
  {
    return millis() - this->usage_start_timestamp;
  }
  return 0;
}

bool Machine::operator==(const Machine &v) const
{
  return (this->config.machine_id.id == v.config.machine_id.id);
}

bool Machine::operator!=(const Machine &v) const
{
  return (this->config.machine_id.id != v.config.machine_id.id);
}

std::string Machine::getMachineName() const
{
  return this->config.machine_name;
}