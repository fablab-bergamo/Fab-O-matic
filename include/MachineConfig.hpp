#ifndef MACHINECONFIG_HPP_
#define MACHINECONFIG_HPP_

#include <chrono>
#include <cstdint>
#include <string>

#include "pins.hpp"
#include "MachineID.hpp"
#include "conf.hpp"

namespace fabomatic
{
  struct MachineConfig
  {
    MachineID machine_id{0};
    MachineType machine_type{MachineType::Invalid};
    std::string machine_name{""};
    const pins_config::relay_config &relay_config;
    const std::string mqtt_switch_topic{""};

    /// @brief Time after which the active user on the machine shall be logged-off
    std::chrono::seconds autologoff{conf::machine::DEFAULT_AUTO_LOGOFF_DELAY};

    /// @brief Time after which the active user on the machine shall be logged-off
    std::chrono::seconds grace_period;

    MachineConfig(MachineID id, MachineType type, const std::string &name,
                  const pins_config::relay_config &relay, const std::string &topic,
                  std::chrono::seconds autologoff,
                  std::chrono::seconds grace_period) : machine_id(id), machine_type(type),
                                                       machine_name(name),
                                                       relay_config{relay},
                                                       mqtt_switch_topic(topic),
                                                       autologoff(autologoff),
                                                       grace_period{grace_period} {};

    [[nodiscard]] auto toString() const -> const std::string;

    /// @brief Indicates if the machine is controller by hard-wired relay
    [[nodiscard]] auto hasRelay() const -> bool;

    /// @brief Indicates if the machine is controller by MQTT switch (Shelly)
    [[nodiscard]] auto hasMqttSwitch() const -> bool;

    MachineConfig() = delete;
    MachineConfig(const MachineConfig &) = default;             // copy constructor
    MachineConfig &operator=(const MachineConfig &x) = default; // copy assignment
    MachineConfig(MachineConfig &&) = delete;                   // move constructor
    MachineConfig &operator=(MachineConfig &&) = delete;        // move assignment
  };
} // namespace fabomatic

#endif // MACHINECONFIG_HPP_