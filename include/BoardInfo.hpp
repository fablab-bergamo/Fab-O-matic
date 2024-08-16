#ifndef BOARDINFO_HPP
#define BOARDINFO_HPP

#include "Machine.hpp"

namespace fabomatic
{
  /// @brief Helper struct representing the display state
  struct BoardInfo
  {
    bool mqtt_connected{false};
    Machine::PowerState power_state{Machine::PowerState::Unknown};
    bool power_warning{false};
    bool unresponsive{true};

    [[nodiscard]] auto operator==(const BoardInfo &other) const -> bool
    {
      return mqtt_connected == other.mqtt_connected &&
             power_state == other.power_state &&
             power_warning == other.power_warning &&
             unresponsive == other.unresponsive;
    };
  };
} // namespace fabomatic
#endif // BOARDINFO_HPP