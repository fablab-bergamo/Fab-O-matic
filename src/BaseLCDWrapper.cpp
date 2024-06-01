
#include <chrono>
#include <string>
#include <sstream>

#include "BaseLCDWrapper.hpp"

namespace fabomatic
{
  using namespace std::chrono;

  auto BaseLCDWrapper::convertSecondsToHHMMSS(std::chrono::seconds duration) const -> const std::string
  {
    std::stringstream ss{};
    const auto hrs = duration_cast<hours>(duration);
    const auto mins = duration_cast<minutes>(duration - hrs);
    const auto secs = duration_cast<seconds>(duration - hrs - mins);

    ss << std::setfill('0') << std::setw(2) << hrs.count() << ":"
       << std::setfill('0') << std::setw(2) << mins.count() << ":"
       << std::setfill('0') << std::setw(2) << secs.count();

    return ss.str();
  }
} // namespace fabomatic