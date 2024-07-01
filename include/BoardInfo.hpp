#ifndef BOARDINFO_HPP
#define BOARDINFO_HPP

#include "Machine.hpp"

namespace fabomatic
{
  /**
   * Helper struct representing the display state
   */
  struct BoardInfo
  {
    bool server_connected;
    Machine::PowerState power_state;
    bool power_warning;

    [[nodiscard]] auto operator==(const BoardInfo &other) const -> bool
    {
      return server_connected == other.server_connected &&
             power_state == other.power_state &&
             power_warning == other.power_warning;
    };
  };
} // namespace fabomatic
#endif // BOARDINFO_HPP