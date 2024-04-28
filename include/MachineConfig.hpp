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
    Invalid = 0,
    Printer3D = 1,
    Laser = 2,
    Cnc = 3,
    PrinterResin = 4,
    Other = 5
  };

  struct MachineID
  {
    uint16_t id;

    constexpr MachineID() : id(0) {}
    constexpr MachineID(uint16_t id) : id(id) {}
    // Add conversion operator to uint16_t to use in print, streams...
    constexpr operator uint16_t() const { return id; }
  };
  struct MachineConfig
  {
    MachineID machine_id{0};
    MachineType machine_type{MachineType::Invalid};
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

    /// @brief Time after which the active user on the machine shall be logged-off
    std::chrono::seconds grace_period;

    MachineConfig(MachineID id, MachineType type, const std::string &name,
                  const pins_config::relay_config &relay, const std::string &topic,
                  std::chrono::seconds autologoff,
                  std::chrono::seconds grace_period) : machine_id(id), machine_type(type),
                                                       machine_name(name),
                                                       relay_config{relay.ch1_pin, relay.active_low},
                                                       mqtt_config{topic}, autologoff(autologoff),
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
} // namespace fablabbg

#endif // MACHINECONFIG_HPP_