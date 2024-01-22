#include "Machine.hpp"
#include "Arduino.h"
#include <sstream>
#include <cstdint>
#include <type_traits>
#include <chrono>
#include "pins.hpp"
#include "secrets.hpp"
#include "FabBackend.hpp"
#include "Logging.hpp"

namespace fablabbg
{
  using namespace std::chrono_literals;
  using namespace std::chrono;

#define CHECK_CONFIGURED(ret_type)                   \
  if (!config.has_value() || !server.has_value())    \
  {                                                  \
    ESP_LOGE(TAG, "Machine : call configure first"); \
    return ret_type();                               \
  }

  void Machine::configure(const MachineConfig &new_config, FabBackend &serv)
  {
    // https://stackoverflow.com/questions/67596731/why-is-stdoptionaltoperator-deleted-when-t-contains-a-const-data-memb
    config.emplace(new_config);

    server = std::reference_wrapper<FabBackend>(serv);

    if (config.value().hasRelay())
    {
      digitalWrite(config.value().relay_config.pin,
                   config.value().relay_config.active_low ? HIGH : LOW);
      pinMode(config.value().relay_config.pin, OUTPUT);
    }

    ESP_LOGD(TAG, "Machine configured : %s", toString().c_str());
  }

  /// @brief Returns the machine identifier
  /// @return Machine identifier
  MachineID Machine::getMachineId() const
  {
    CHECK_CONFIGURED(MachineID);
    return config.value().machine_id;
  }

  /// @brief Indicates whether the machine is used by somebody
  /// @return boolean
  bool Machine::isFree() const
  {
    return !active;
  }

  /// @brief Log the given user onto the machine, if free and not blocked
  /// @param user user to login
  /// @return true of the user has been successfully logged in
  bool Machine::login(FabUser user)
  {
    CHECK_CONFIGURED(bool);
    if (isFree() && allowed)
    {
      active = true;
      current_user = user;
      power(true);
      usage_start_timestamp = system_clock::now();
      return true;
    }
    return false;
  }

  /// @brief Returns the current power state of the machine
  /// @return
  Machine::PowerState Machine::getPowerState() const
  {
    return power_state;
  }

  /// @brief Removes the user from the machine, and powers off the machine (respecting POWEROFF_DELAY_MINUTES setting)
  void Machine::logout()
  {
    CHECK_CONFIGURED(void);
    if (active)
    {
      active = false;
      power_state = PowerState::WAITING_FOR_POWER_OFF;
      usage_start_timestamp = std::nullopt;

      // Sets the countdown to power off
      logoff_timestamp = system_clock::now();

      if (conf::machine::POWEROFF_GRACE_PERIOD == 0s)
      {
        power(false);
      }

      ESP_LOGI(TAG, "Machine will be shutdown in %lld s",
               duration_cast<seconds>(conf::machine::POWEROFF_GRACE_PERIOD).count());
    }
  }

  /// @brief indicates if the machine can be powered off
  /// @return true if the delay has expired
  bool Machine::canPowerOff() const
  {
    if (!logoff_timestamp.has_value())
      return false;

    return (power_state == PowerState::WAITING_FOR_POWER_OFF &&
            system_clock::now() - logoff_timestamp.value() > conf::machine::POWEROFF_GRACE_PERIOD);
  }

  /// @brief indicates if the machine is about to shudown and board should beep
  /// @return true if shutdown is imminent
  bool Machine::isShutdownImminent() const
  {
    if (!logoff_timestamp.has_value() || conf::machine::BEEP_PERIOD == 0ms)
      return false;

    return (power_state == PowerState::WAITING_FOR_POWER_OFF &&
            system_clock::now() - logoff_timestamp.value() > conf::machine::BEEP_PERIOD);
  }

  /// @brief sets the machine power to on (true) or off (false)
  /// @param value setpoint
  void Machine::power_relay(bool value)
  {
    CHECK_CONFIGURED(void);
    ESP_LOGI(TAG, "Machine::power_relay : power set to %d", value);

    auto pin = config.value().relay_config.pin;

    if (config.value().relay_config.active_low)
    {
      digitalWrite(pin, value ? LOW : HIGH);
    }
    else
    {
      digitalWrite(pin, value ? HIGH : LOW);
    }

    if (value)
    {
      logoff_timestamp = std::nullopt;
      power_state = PowerState::POWERED_ON;
    }
    else
    {
      power_state = PowerState::POWERED_OFF;
    }
  }

  /// @brief sets the machine power to on (true) or off (false)
  /// @param value setpoint
  void Machine::power_mqtt(bool value)
  {
    CHECK_CONFIGURED(void);

    ESP_LOGI(TAG, "Machine::power_mqtt : power set to %d", value);

    auto &mqtt_server = server.value().get();
    auto &act_config = config.value();

    String topic{act_config.mqtt_config.topic.data()};
    String payload = value ? act_config.mqtt_config.on_message.data() : act_config.mqtt_config.off_message.data();

    auto retries = 0;
    constexpr auto DELAY_MS = duration_cast<milliseconds>(conf::mqtt::TIMEOUT_REPLY_SERVER).count();
    while (!mqtt_server.publish(topic, payload))
    {
      ESP_LOGE(TAG, "Error while publishing %s to %s", payload.c_str(), topic.c_str());

      mqtt_server.connect();
      delay(DELAY_MS);
      retries++;
      if (retries > conf::mqtt::MAX_TRIES)
      {
        ESP_LOGW(TAG, "Unable to publish to MQTT (%d/%d)", retries, conf::mqtt::MAX_TRIES);
        return;
      }
    }

    if (value)
    {
      logoff_timestamp = std::nullopt;
      power_state = PowerState::POWERED_ON;
    }
    else
    {
      power_state = PowerState::POWERED_OFF;
    }
  }

  void Machine::power(bool on_or_off)
  {
    CHECK_CONFIGURED(void);

    ESP_LOGI(TAG, "Machine::power : power set to %d", on_or_off);

    if (config.value().hasRelay())
    {
      power_relay(on_or_off);
    }
    if (config.value().hasMqttSwitch())
    {
      power_mqtt(on_or_off);
    }
  }

  FabUser Machine::getActiveUser() const
  {
    return current_user;
  }

  /// @brief Gets the duration the machine has been used
  /// @return milliseconds since the machine has been started
  seconds Machine::getUsageDuration() const
  {
    if (usage_start_timestamp.has_value())
    {
      return duration_cast<seconds>(system_clock::now() - usage_start_timestamp.value());
    }
    return 0s;
  }

  std::string Machine::getMachineName() const
  {
    CHECK_CONFIGURED(std::string);
    return std::string{config.value().machine_name.data()};
  }

  std::string Machine::toString() const
  {
    std::stringstream sstream;

    if (!config.has_value() || !server.has_value())
    {
      sstream << "Machine (not configured)";
      return sstream.str();
    }

    sstream << "Machine (ID:" << getMachineId().id;
    sstream << ", Name:" << getMachineName();
    sstream << ", IsFree: " << isFree();
    sstream << ", IsAllowed:" << allowed;
    sstream << ", PowerState:" << static_cast<int>(getPowerState());
    sstream << ", " << current_user.toString();
    sstream << ", UsageDuration (s):" << getUsageDuration().count();
    sstream << ", ShutdownImminent:" << isShutdownImminent();
    sstream << ", MaintenanceNeeded:" << maintenanceNeeded;
    sstream << ", " << config.value().toString();
    sstream << ", Active:" << active;
    sstream << ", Last logoff:" << (logoff_timestamp.has_value() ? logoff_timestamp.value().time_since_epoch().count() : 0);
    sstream << ")";

    return sstream.str();
  }

  seconds Machine::getAutologoffDelay() const
  {
    CHECK_CONFIGURED(seconds);
    return config.value().autologoff;
  }

  void Machine::setAutologoffDelay(seconds new_delay)
  {
    CHECK_CONFIGURED(void);

    if (config.value().autologoff != new_delay)
    {
      ESP_LOGD(TAG, "Changing autologoff delay to %lld min",
               duration_cast<minutes>(config.value().autologoff).count());
    }

    config.value().autologoff = new_delay;
  }

  bool Machine::isAutologoffExpired() const
  {
    return getAutologoffDelay() > 0min &&
           getUsageDuration() > getAutologoffDelay();
  }

  void Machine::setMachineName(const std::string &new_name)
  {
    CHECK_CONFIGURED(void);

    if (config.value().machine_name != new_name)
    {
      ESP_LOGD(TAG, "Changing machine name to %s", new_name.data());
    }

    config.value().machine_name = new_name;
  }

  void Machine::setMachineType(MachineType new_type)
  {
    CHECK_CONFIGURED(void);

    if (config.value().machine_type != new_type)
      ESP_LOGD(TAG, "Changing machine type to %d", static_cast<uint8_t>(new_type));

    config.value().machine_type = new_type;
  }

  bool Machine::isConfigured() const
  {
    return config.has_value() && server.has_value();
  }

  std::optional<MachineConfig> Machine::getConfig() const
  {
    return config;
  }
} // namespace fablabbg