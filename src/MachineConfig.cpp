#include "MachineConfig.hpp"
#include <sstream>

namespace fablabbg
{
  std::string MachineConfig::toString() const
  {
    std::stringstream sstream;

    sstream << "MachineConfig (ID:" << this->machine_id.id;
    sstream << ", Name:" << this->machine_name;
    sstream << ", Type:" << static_cast<int>(this->machine_type);
    sstream << ", RelayConfig (pin:" << static_cast<int>(this->relay_config.pin);
    sstream << ", active_low:" << this->relay_config.active_low;
    sstream << "), MQTTConfig (topic:" << this->mqtt_config.topic;
    sstream << ", on_message:" << this->mqtt_config.on_message;
    sstream << ", off_message:" << this->mqtt_config.off_message;
    sstream << "), AutologoffDelay (min):" << this->autologoff.count();
    sstream << ", HasRelay:" << this->hasRelay();
    sstream << ", HasMqttSwitch:" << this->hasMqttSwitch();
    sstream << "))";

    return sstream.str();
  }

  bool MachineConfig::hasRelay() const
  {
    return this->relay_config.pin != NO_PIN;
  }

  bool MachineConfig::hasMqttSwitch() const
  {
    return !this->mqtt_config.topic.empty();
  }
}