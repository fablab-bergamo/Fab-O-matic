#ifndef BASELCDWRAPPER_H_
#define BASELCDWRAPPER_H_

#include <string>
#include "BoardInfo.hpp"
#include "Machine.hpp"
#include <chrono>

using namespace std::chrono;

namespace fablabbg
{
  class BaseLCDWrapper
  {
  public:
    virtual bool begin() = 0;
    virtual void clear() = 0;
    virtual void showConnection(bool show) = 0;
    virtual void showPower(bool show) = 0;
    virtual void setRow(uint8_t row, const std::string &text) = 0;
    std::string convertSecondsToHHMMSS(duration<uint16_t> duration) const;
    virtual void update(const BoardInfo &boardinfo, bool forced = false) = 0;
  };
} // namespace fablabbg

#endif // BASELCDWRAPPER_H_
