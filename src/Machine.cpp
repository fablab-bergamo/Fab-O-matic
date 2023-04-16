#include "Machine.h"
#include "Arduino.h"

#include <cstdint>

/// @brief Creates a new machine
/// @param user_conf configuration of the machine
Machine::Machine(Config user_conf) : config(user_conf), active(false), usage_start_timestamp(0), maintenanceNeeded(false), allowed(true)
{
  this->current_user = FabUser();
  pinMode(this->config.control_pin, OUTPUT);
  Serial.printf("Machine %s configured on pin %d (active_low:%d)\n", this->config.machine_name.c_str(), this->config.control_pin, this->config.control_pin_active_low);
  this->power(false);
}

/// @brief Returns the machine identifier
/// @return Machine identifier
Machine::MachineID Machine::getMachineId() const
{
  return this->config.machine_id;
}

/// @brief Indicates whether the machine is used by somebody
/// @return boolean
bool Machine::isFree() const
{
  return !this->active;
}

/// @brief Log the given user onto the machine, if free and not blocked
/// @param user user to login
/// @return true of the user has been successfully logged in
bool Machine::login(FabUser user)
{
  if (this->isFree() && this->allowed)
  {
    this->active = true;
    this->current_user = user;
    this->power(true);
    this->usage_start_timestamp = millis();
    return true;
  }
  return false;
}

/// @brief Returns the current power state of the machine
/// @return
Machine::PowerState Machine::getPowerState() const
{
  return this->powerState;
}

/// @brief Removes the user from the machine, and powers off the machine (respecting POWEROFF_DELAY_MINUTES setting)
void Machine::logout()
{
  if (this->active)
  {
    active = false;
    this->powerState = PowerState::WAITING_FOR_POWER_OFF;

    // Sets the countdown to power off
    if (conf::machine::POWEROFF_DELAY_MINUTES > 0)
    {
      this->logout_timestamp = millis();
      Serial.printf("Machine will be shutdown in %d minutes\n", conf::machine::POWEROFF_DELAY_MINUTES);
    }
    else
    {
      this->logout_timestamp = 0;
      this->power(false);
    }
  }
}

bool Machine::canPowerOff() const
{
  if (this->logout_timestamp == 0)
    return false;

  return (this->powerState == PowerState::WAITING_FOR_POWER_OFF &&
          millis() - this->logout_timestamp > conf::machine::POWEROFF_DELAY_MINUTES * 60 * 1000);
}

bool Machine::isShutdownPending() const
{
  if (this->logout_timestamp == 0)
    return false;

  auto beep_ts = this->logout_timestamp - (conf::machine::BEEP_REMAINING_MINUTES * 60 * 1000);

  return (this->powerState == PowerState::WAITING_FOR_POWER_OFF &&
          millis() - this->logout_timestamp > conf::machine::BEEP_REMAINING_MINUTES * 60 * 1000);
}

void Machine::power(bool value)
{
  Serial.printf("Power set to %d\n", value);
  if (this->config.control_pin_active_low)
  {
    digitalWrite(this->config.control_pin, value ? LOW : HIGH);
  }
  else
  {
    digitalWrite(this->config.control_pin, value ? HIGH : LOW);
  }

  if (value)
  {
    this->logout_timestamp = 0;
    this->powerState = PowerState::POWERED_ON;
  }
  else
  {
    this->powerState = PowerState::POWERED_OFF;
  }
}

FabUser &Machine::getActiveUser()
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