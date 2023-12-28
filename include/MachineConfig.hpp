#ifndef MACHINE_CONFIG_H_
#define MACHINE_CONFIG_H_

#include <cstdint>
#include <string>
#include <chrono>
#include "pins.hpp"

using namespace std::chrono;

namespace fablabbg
{
  enum class MachineType : uint8_t
  {
    INVALID = 0,
    PRINTER3D = 1,
    LASER = 2,
    CNC = 3,
    UNKNOWN = 4
  };

  struct MachineID
  {
    uint16_t id;
  };
  struct MachineConfig
  {
    MachineID machine_id{0};
    MachineType machine_type{MachineType::INVALID};
    std::string machine_name;
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

    minutes autologoff;

    MachineConfig(MachineID id, MachineType type, std::string_view name,
                  uint8_t pin, bool act_low, std::string_view topic,
                  minutes autologoff) : machine_id(id), machine_type(type), machine_name(name),
                                        relay_config{pin, act_low},
                                        mqtt_config{std::string{topic}},
                                        autologoff(autologoff) {}
    std::string toString() const;
    bool hasRelay() const;
    bool hasMqttSwitch() const;

    MachineConfig() = delete;
    MachineConfig(const MachineConfig &) = default;             // copy constructor
    MachineConfig &operator=(const MachineConfig &x) = default; // copy assignment
    MachineConfig(MachineConfig &&) = delete;                   // move constructor
    MachineConfig &operator=(MachineConfig &&) = delete;        // move assignment
  };
}

#endif // MACHINE_CONFIG_H_