#include "LiquidCrystal.h"
#include <array>  

#ifndef _LCD_WRAPPER_H_
#define _LCD_WRAPPER_H_

namespace LCDWrapper {
  typedef enum {
    CLEAR,
    FREE,
    LOGGED_IN,
    LOGIN_DENIED,
    BUSY,
    LOGOUT,
    CONNECTING,
    CONNECTED
  } LCDStateType;
}

template <uint8_t _COLS, uint8_t _ROWS>
class LCDWrapperClass
{
private:
  LiquidCrystal _lcd;

  LCDWrapper::LCDStateType _state;

  uint8_t _backlight_pin;
  bool _backlight_active_low;

  bool _show_connection_status;
  bool _connection_status;

  uint8_t _rows = _ROWS;
  uint8_t _cols = _COLS;

  void _backlightOn();
  void _backlightOff();

  std::array<std::array<char, _COLS>,  _ROWS> _buffer;
  std::array<std::array<char, _COLS>,  _ROWS> _current;

  bool _needsUpdate();
  void _update_chars();

  uint8_t _antenna_char[8] = {0x15, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
  uint8_t _connection_char[8] = {0x00, 0x00, 0x01, 0x01, 0x05, 0x05, 0x15, 0x15};
  uint8_t _noconnection_char[8] = {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00};


public:    
  LCDWrapperClass(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t backlight_pin, bool backlight_active_low);
  LCDWrapperClass(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);

  void begin();
  
  void update();
  void clear();

  void state(LCDWrapper::LCDStateType state);

  void setRow(uint8_t row, const char* text);

  void setConnectionState(bool connected);
  void showConnection(bool show);

  
};

#include "LCDWrapperClass.tpp"
#endif // _LCD_WRAPPER_H_
