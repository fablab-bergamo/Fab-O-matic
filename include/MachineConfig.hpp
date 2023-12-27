#ifndef MACHINE_CONFIG_H_
#define MACHINE_CONFIG_H_

#include <cstdint>
#include <string>
#include "pins.hpp"
#include "conf.hpp"

namespace fablabbg
{
  enum class MachineType
  {
    INVALID,
    PRINTER3D,
    LASER,
    CNC,
    EXTRA1,
    EXTRA2
  };
  struct MachineID
  {
    const uint16_t id;
  };
  struct MachineConfig
  {
    const MachineID machine_id{0};
    const MachineType machine_type{MachineType::INVALID};
    const std::string machine_name;
    struct RelayConfig
    {
      const uint8_t pin{NO_PIN};
      const bool active_low{false};
    } relay_config;
    struct MQTTConfig
    {
      const std::string topic{""};
      const std::string on_message{"on"};
      const std::string off_message{"off"};
    } mqtt_config;

    std::chrono::minutes autologoff{conf::machine::DEFAULT_AUTO_LOGOFF_DELAY};

    MachineConfig(MachineID id, MachineType type, std::string_view name,
                  uint8_t pin, bool act_low, std::string_view topic,
                  std::chrono::minutes autologoff) : machine_id(id), machine_type(type), machine_name(name),
                                                     relay_config{pin, act_low},
                                                     mqtt_config{std::string{topic}},
                                                     autologoff(autologoff) {}
    std::string toString() const;
    bool hasRelay() const;
    bool hasMqttSwitch() const;
  };
}

#endif // MACHINE_CONFIG_H_