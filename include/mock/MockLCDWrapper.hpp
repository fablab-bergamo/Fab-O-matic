#ifndef MOCKLCDWRAPPER_H_
#define MOCKLCDWRAPPER_H_

#include <array>
#include "BoardInfo.hpp"
#include "LiquidCrystal.h"
#include "Machine.hpp"
#include "pins.hpp"
#include <chrono>
#include "BaseLCDWrapper.hpp"

using namespace std::chrono;

namespace fablabbg
{
  class MockLCDWrapper : public BaseLCDWrapper
  {
  public:
    bool begin();
    void clear();
    void showConnection(bool show);
    void showPower(bool show);
    void setRow(uint8_t row, const std::string_view text);
    void update(const BoardInfo &boardinfo, bool forced = false);

    // Mockup utils
    BoardInfo last_boardinfo;
  };
} // namespace fablabbg

#endif // MOCKLCDWRAPPER_H_
