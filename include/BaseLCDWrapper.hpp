#ifndef BASELCDWRAPPER_H_
#define BASELCDWRAPPER_H_

#include <array>
#include "BoardInfo.hpp"
#include "Machine.hpp"
#include <chrono>

using namespace std::chrono;

namespace fablabbg
{
  class BaseLCDWrapper
  {
  public:
    virtual bool begin();
    virtual void clear();
    virtual void showConnection(bool show);
    virtual void showPower(bool show);
    virtual void setRow(uint8_t row, const std::string_view text);
    std::string convertSecondsToHHMMSS(duration<uint16_t> duration) const;
    virtual void update(const BoardInfo &boardinfo, bool forced = false);
  };
} // namespace fablabbg

#endif // BASELCDWRAPPER_H_
