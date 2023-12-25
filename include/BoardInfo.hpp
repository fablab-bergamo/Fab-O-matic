#ifndef BOARDINFO_HPP
#define BOARDINFO_HPP

#include "Machine.hpp"

namespace fablabbg
{
  struct BoardInfo
  {
    bool server_connected;
    Machine::PowerState power_state;
    bool power_warning;
    bool operator==(const BoardInfo &t) const
    {
      return server_connected == t.server_connected &&
            power_state == t.power_state &&
            power_warning == t.power_warning;
    }
  };
}
#endif // BOARDINFO_HPP