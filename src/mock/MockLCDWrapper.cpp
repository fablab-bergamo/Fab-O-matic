#include "mock/MockLCDWrapper.hpp"

namespace fablabbg
{
  bool MockLCDWrapper::begin()
  {
    return true;
  }

  void MockLCDWrapper::clear()
  {
  }

  void MockLCDWrapper::showConnection(bool show)
  {
  }

  void MockLCDWrapper::showPower(bool show)
  {
  }

  void MockLCDWrapper::setRow(uint8_t row, const std::string_view text)
  {
  }

  void MockLCDWrapper::update(const BoardInfo &boardinfo, bool forced)
  {
    last_boardinfo = boardinfo;
  }
} // namespace fablabbg