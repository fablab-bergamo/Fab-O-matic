#ifndef _LCD_WRAPPER_H_
#define _LCD_WRAPPER_H_

#include "LiquidCrystal.h"
#include <array>

namespace LCDState
{
  typedef enum
  {
    CLEAR = 0x0,
    FREE = 0x1,
    LOGGED_IN = 0x2,
    LOGIN_DENIED = 0x3,
    BUSY = 0x4,
    LOGOUT = 0x5,
    CONNECTING = 0x6,
    CONNECTED = 0x7,
    ALREADY_IN_USE = 0x8,
    IN_USE = 0x9,
    OFFLINE = 0x10
  } LCDStateType;
}

template <uint8_t _COLS, uint8_t _ROWS>
class LCDWrapper
{
private:
  LiquidCrystal _lcd;

  LCDState::LCDStateType _state;

  uint8_t _backlight_pin;
  bool _backlight_active_low;

  bool _show_connection_status;
  bool _connection_status;

  void _backlightOn();
  void _backlightOff();

  std::array<std::array<char, _COLS>, _ROWS> _buffer;
  std::array<std::array<char, _COLS>, _ROWS> _current;

  bool _needsUpdate();
  void _update_chars();

  uint8_t _antenna_char[8] = {0x15, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
  uint8_t _connection_char[8] = {0x00, 0x00, 0x01, 0x01, 0x05, 0x05, 0x15, 0x15};
  uint8_t _noconnection_char[8] = {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00};

  void setRow(uint8_t row, std::string text);
  std::string convertSecondsToHHMMSS(unsigned long millis);

public:

  LCDWrapper(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t backlight_pin, bool backlight_active_low);
  LCDWrapper(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);

  void begin();

  void update(FabServer server, FabMember user, Machine machine);
  void clear();
  void state(LCDState::LCDStateType state);
  void setConnectionState(bool connected);
  void showConnection(bool show);
};

#include "LCDWrapper.tpp"
#endif // _LCD_WRAPPER_H_
