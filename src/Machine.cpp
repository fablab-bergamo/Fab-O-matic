#include "Machine.hpp"
#include "Arduino.h"
#include <sstream>
#include <cstdint>
#include <type_traits>
#include <chrono>
#include "pins.hpp"
#include "secrets.hpp"
#include "FabServer.hpp"

namespace fablabbg
{
  using namespace std::chrono_literals;
  using namespace std::chrono;

  /// @brief Creates a new machine
  Machine::Machine() : maintenanceNeeded(false), allowed(true),
                       config(std::nullopt), server(std::nullopt),
                       active(false), power_state(PowerState::UNKNOWN)

  {
    this->current_user = FabUser();
  }

  void Machine::configure(MachineConfig new_config, FabServer &serv)
  {
    this->config = new_config;
    this->server = std::reference_wrapper<FabServer>(serv);
    if (this->config.value().hasRelay())
    {
      pinMode(this->config.value().relay_config.pin, OUTPUT);
    }
  }

  /// @brief Returns the machine identifier
  /// @return Machine identifier
  MachineID Machine::getMachineId() const
  {
    if (!this->config.has_value())
    {
      return MachineID{0};
    }
    return this->config.value().machine_id;
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
      this->usage_start_timestamp = system_clock::now();
      return true;
    }
    return false;
  }

  /// @brief Returns the current power state of the machine
  /// @return
  Machine::PowerState Machine::getPowerState() const
  {
    return this->power_state;
  }

  /// @brief Removes the user from the machine, and powers off the machine (respecting POWEROFF_DELAY_MINUTES setting)
  void Machine::logout()
  {
    if (this->active)
    {
      this->active = false;
      this->power_state = PowerState::WAITING_FOR_POWER_OFF;
      this->usage_start_timestamp = std::nullopt;

      // Sets the countdown to power off
      this->logoff_timestamp = system_clock::now();

      if (conf::machine::POWEROFF_GRACE_PERIOD == 0s)
      {
        this->power(false);
      }

      if (conf::debug::ENABLE_LOGS)
        Serial.printf("Machine will be shutdown in %lld s\r\n",
                      duration_cast<seconds>(conf::machine::POWEROFF_GRACE_PERIOD).count());
    }
  }

  /// @brief indicates if the machine can be powered off
  /// @return true if the delay has expired
  bool Machine::canPowerOff() const
  {
    if (!this->logoff_timestamp.has_value())
      return false;

    return (this->power_state == PowerState::WAITING_FOR_POWER_OFF &&
            system_clock::now() - this->logoff_timestamp.value() > conf::machine::POWEROFF_GRACE_PERIOD);
  }

  /// @brief indicates if the machine is about to shudown and board should beep
  /// @return true if shutdown is imminent
  bool Machine::isShutdownImminent() const
  {
    if (!this->logoff_timestamp.has_value() || conf::machine::BEEP_PERIOD == 0ms)
      return false;

    return (this->power_state == PowerState::WAITING_FOR_POWER_OFF &&
            system_clock::now() - this->logoff_timestamp.value() > conf::machine::BEEP_PERIOD);
  }

  /// @brief sets the machine power to on (true) or off (false)
  /// @param value setpoint
  void Machine::power_relay(bool value)
  {
    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Machine::power_relay : power set to %d\r\n", value);

    if (!this->config.has_value())
    {
      Serial.println("Machine::power_relay : machine is not configured");
      return;
    }

    auto pin = this->config.value().relay_config.pin;

    if (this->config.value().relay_config.active_low)
    {
      digitalWrite(pin, value ? LOW : HIGH);
    }
    else
    {
      digitalWrite(pin, value ? HIGH : LOW);
    }

    if (value)
    {
      this->logoff_timestamp = std::nullopt;
      this->power_state = PowerState::POWERED_ON;
    }
    else
    {
      this->power_state = PowerState::POWERED_OFF;
    }
  }

  /// @brief sets the machine power to on (true) or off (false)
  /// @param value setpoint
  void Machine::power_mqtt(bool value)
  {
    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Machine::power_mqtt : power set to %d\r\n", value);

    if (!this->server.has_value() || !this->config.has_value())
    {
      Serial.println("Machine::power_mqtt : server is not configured");
      return;
    }
    auto &mqtt_server = this->server.value().get();
    auto &act_config = this->config.value();

    String topic{act_config.mqtt_config.topic.data()};
    String payload = value ? act_config.mqtt_config.on_message.data() : act_config.mqtt_config.off_message.data();

    auto retries = 0;
    while (!mqtt_server.publish(topic, payload))
    {
      if (conf::debug::ENABLE_LOGS)
        Serial.printf("Error while publishing %s to %s\r\n", payload.c_str(), topic.c_str());

      mqtt_server.connect();
      delay(500);
      retries++;
      if (retries > 5)
      {
        Serial.println("Unable to publish to MQTT");
        return;
      }
    }

    if (value)
    {
      this->logoff_timestamp = std::nullopt;
      this->power_state = PowerState::POWERED_ON;
    }
    else
    {
      this->power_state = PowerState::POWERED_OFF;
    }
  }

  void Machine::power(bool on_or_off)
  {
    if (conf::debug::ENABLE_LOGS)
      Serial.printf("Machine::power : power set to %d\r\n", on_or_off);

    if (!this->config.has_value())
    {
      Serial.println("Machine::power : machine is not configured");
      return;
    }

    if (this->config.value().hasRelay())
    {
      this->power_relay(on_or_off);
    }
    if (this->config.value().hasMqttSwitch())
    {
      this->power_mqtt(on_or_off);
    }
  }

  FabUser &Machine::getActiveUser()
  {
    return this->current_user;
  }

  /// @brief Gets the duration the machine has been used
  /// @return milliseconds since the machine has been started
  seconds Machine::getUsageDuration() const
  {
    if (this->usage_start_timestamp.has_value())
    {
      return duration_cast<seconds>(system_clock::now() - this->usage_start_timestamp.value());
    }
    return 0s;
  }

  std::string Machine::getMachineName() const
  {
    if (!this->config.has_value())
    {
      return "Machine not configured";
    }
    return this->config.value().machine_name;
  }

  std::string Machine::toString() const
  {
    std::stringstream sstream;

    sstream << "Machine (ID:" << this->getMachineId().id;
    sstream << ", Name:" << this->getMachineName();
    sstream << ", IsFree: " << this->isFree();
    sstream << ", IsAllowed:" << this->allowed;
    sstream << ", PowerState:" << static_cast<int>(this->getPowerState());
    sstream << ", " << this->current_user.toString();
    sstream << ", UsageDuration (s):" << this->getUsageDuration().count();
    sstream << ", ShutdownImminent:" << this->isShutdownImminent();
    sstream << ", MaintenanceNeeded:" << this->maintenanceNeeded;

    if (this->config.has_value())
    {
      sstream << ", " << this->config.value().toString();
    }
    else
    {
      sstream << ", No config";
    }
    sstream << ", Active:" << this->active;
    sstream << ", Last logoff:" << (this->logoff_timestamp.has_value() ? this->logoff_timestamp.value().time_since_epoch().count() : 0);
    sstream << ")";

    return sstream.str();
  }

  minutes Machine::getAutologoffDelay() const
  {
    if (!this->config.has_value())
    {
      return 0min;
    }
    return this->config.value().autologoff;
  }

  void Machine::setAutologoffDelay(minutes new_delay)
  {
    if (!this->config.has_value())
    {
      Serial.println("Machine::setAutologoffDelay : machine is not configured");
      return;
    }

    if (conf::debug::ENABLE_LOGS && this->config.value().autologoff != new_delay)
      Serial.printf("Setting autologoff delay to %lld min\r\n", new_delay.count());

    this->config.value().autologoff = new_delay;
  }

  bool Machine::isAutologoffExpired() const
  {
    return this->getAutologoffDelay() > 0min &&
           this->getUsageDuration() > this->getAutologoffDelay();
  }

  void Machine::setMachineName(std::string_view new_name)
  {
    if (!this->config.has_value())
    {
      Serial.println("Machine::setMachineName : machine is not configured");
      return;
    }

    if (conf::debug::ENABLE_LOGS && this->config.value().machine_name != new_name)
      Serial.printf("Setting machine name to %s\r\n", new_name.data());

    this->config.value().machine_name = new_name;
  }

  void Machine::setMachineType(MachineType new_type)
  {
    if (!this->config.has_value())
    {
      Serial.println("Machine::setMachineType : machine is not configured");
      return;
    }

    if (conf::debug::ENABLE_LOGS && this->config.value().machine_type != new_type)
      Serial.printf("Setting machine type to %d\r\n", static_cast<uint8_t>(new_type));

    this->config.value().machine_type = new_type;
  }
} // namespace fablabbg