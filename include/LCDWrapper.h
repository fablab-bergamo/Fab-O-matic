#ifndef _LCD_WRAPPER_H_
#define _LCD_WRAPPER_H_

#include <array>
#include "LiquidCrystal.h"
#include "BoardState.h"
#include "Machine.h"

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

template <uint8_t _COLS, uint8_t _ROWS>
class LCDWrapper
{
public:
  struct Config
  {
    uint8_t rs;
    uint8_t enable;
    uint8_t d0;
    uint8_t d1;
    uint8_t d2;
    uint8_t d3;
    uint8_t backlight_pin;
    bool backlight_active_low;
    Config(uint8_t rs, uint8_t en, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t backlight_pin = -1, bool backlight_active_low = false) : d0(d0), d1(d1), d2(d2), d3(d3), backlight_pin(backlight_pin), backlight_active_low(backlight_active_low), enable(en), rs(rs){};
  };

  LCDWrapper(Config config);

  void begin();
  void clear();
  void showConnection(bool show);
  void showPower(bool show);
  void setRow(uint8_t row, std::string text);
  std::string convertSecondsToHHMMSS(unsigned long millis);
  void update_chars(BoardInfo boardinfo);

private:
  static constexpr uint8_t CHAR_ANTENNA = 0;
  static constexpr uint8_t CHAR_CONNECTION = 1;
  static constexpr uint8_t CHAR_NO_CONNECTION = 2;
  static constexpr uint8_t CHAR_POWERED_ON = 3;
  static constexpr uint8_t CHAR_POWERED_OFF = 4;
  static constexpr uint8_t CHAR_POWERING_OFF = 5;
  const Config config;

  LiquidCrystal lcd;
  bool show_connection_status;
  bool show_power_status;

  void backlightOn();
  void backlightOff();

  std::array<std::array<char, _COLS>, _ROWS> buffer;
  std::array<std::array<char, _COLS>, _ROWS> current;
  BoardInfo boardInfo;

  bool needsUpdate(BoardInfo bi);
  bool forceUpdate;
  void pretty_print(std::array<std::array<char, _COLS>, _ROWS> buffer);

  // Ideally static constexpr, but the LiquidCrystal library expects non-const uint8_t for char definitions
  uint8_t antenna_char[8] = {0x15, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
  uint8_t connection_char[8] = {0x00, 0x00, 0x01, 0x01, 0x05, 0x05, 0x15, 0x15};
  uint8_t noconnection_char[8] = {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00};
  uint8_t powered_on_char[8] = {0x04, 0x04, 0x04, 0x1f, 0x1f, 0x1f, 0x0a, 0x0a};
  uint8_t powered_off_char[8] = {0x0a, 0x04, 0x0a, 0x00, 0x1f, 0x1f, 0x0a, 0x0a};
  uint8_t powering_off_char[8] = {0x0e, 0x15, 0x15, 0x15, 0x17, 0x11, 0x11, 0x0e};
};

#include "LCDWrapper.tpp"
#endif // _LCD_WRAPPER_H_
