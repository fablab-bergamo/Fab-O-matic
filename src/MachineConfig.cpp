#include "MachineConfig.hpp"
#include <sstream>

namespace fabomatic
{
  auto MachineConfig::toString() const -> const std::string
  {
    std::stringstream sstream{};

    sstream << "MachineConfig (ID:" << machine_id.id;
    sstream << ", Name:" << machine_name;
    sstream << ", Type:" << static_cast<int>(machine_type);
    sstream << ", AutologoffDelay (min):" << std::chrono::duration_cast<std::chrono::minutes>(autologoff).count();
    sstream << ", MQTT Switch topic:" << mqtt_switch_topic;
    sstream << ", HasRelay:" << hasRelay();
    sstream << ", HasMqttSwitch:" << hasMqttSwitch();
    sstream << ", GracePeriod (s):" << std::chrono::duration_cast<std::chrono::seconds>(grace_period).count();
    sstream << "))";

    return sstream.str();
  }

  auto MachineConfig::hasRelay() const -> bool
  {
    return relay_config.ch1_pin != NO_PIN;
  }

  auto MachineConfig::hasMqttSwitch() const -> bool
  {
    return !mqtt_switch_topic.empty();
  }
} // namespace fabomatic