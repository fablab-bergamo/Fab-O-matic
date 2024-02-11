#ifndef MACHINECONFIG_HPP_
#define MACHINECONFIG_HPP_

#include "pins.hpp"
#include <chrono>
#include <cstdint>
#include <string>

namespace fablabbg
{
  enum class MachineType : uint8_t
  {
    INVALID = 0,
    PRINTER3D = 1,
    LASER = 2,
    CNC = 3,
    UNKNOWN = 4,
  };

  struct MachineID
  {
    uint16_t id;
  };
  struct MachineConfig
  {
    MachineID machine_id{0};
    MachineType machine_type{MachineType::INVALID};
    std::string machine_name{""};
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

    /// @brief Time after which the active user on the machine shall be logged-off
    std::chrono::seconds autologoff;

    MachineConfig(MachineID id, MachineType type, const std::string &name,
                  const pins_config::relay_config &relay, const std::string &topic,
                  std::chrono::seconds autologoff) : machine_id(id), machine_type(type),
                                                     machine_name(name),
                                                     relay_config{relay.ch1_pin, relay.active_low},
                                                     mqtt_config{topic},
                                                     autologoff(autologoff){};
    std::string toString() const;

    /// @brief Indicates if the machine is controller by hard-wired relay
    bool hasRelay() const;

    /// @brief Indicates if the machine is controller by MQTT switch (Shelly)
    bool hasMqttSwitch() const;

    MachineConfig() = delete;
    MachineConfig(const MachineConfig &) = default;             // copy constructor
    MachineConfig &operator=(const MachineConfig &x) = default; // copy assignment
    MachineConfig(MachineConfig &&) = delete;                   // move constructor
    MachineConfig &operator=(MachineConfig &&) = delete;        // move assignment
  };
} // namespace fablabbg

#endif // MACHINECONFIG_HPP_