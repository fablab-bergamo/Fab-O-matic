
#include <string>
#include <chrono>

#include "BaseLCDWrapper.hpp"

namespace fablabbg
{
  std::string BaseLCDWrapper::convertSecondsToHHMMSS(duration<uint16_t> duration) const
  {
    //! since something something does not support to_string we have to resort to ye olde cstring stuff
    char buf[9] = {0};

    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
             duration.count() / 3600UL,
             (duration.count() % 3600UL) / 60UL,
             duration.count() % 60UL);

    return {buf};
  }
}