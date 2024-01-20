#ifndef MOCK_LCDLIBRARY_HPP_
#define MOCK_LCDLIBRARY_HPP_

#include <stdint.h>

namespace fablabbg
{
  class MockLcdLibrary
  {
  public:
    constexpr MockLcdLibrary() = default;
    MockLcdLibrary(uint8_t rs_pin, uint8_t en_pin, uint8_t d0_pin, uint8_t d1_pin, uint8_t d2_pin, uint8_t d3_pin){};
    void begin(uint8_t cols, uint8_t rows){};
    void createChar(uint8_t num, const uint8_t *values){};
    void clear(){};
    void print(const char *text){};
    void setCursor(uint8_t col, uint8_t row){};
    void write(uint8_t value){};
  };
} // namespace fablabbg

#endif // MOCK_LCDLIBRARY_HPP_