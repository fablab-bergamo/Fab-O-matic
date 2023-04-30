#ifndef LCDWRAPPER_H_
#define LCDWRAPPER_H_

#include <array>
#include "LiquidCrystal.h"
#include "Machine.h"
#include "pins.h"
#include <chrono>

using namespace std::chrono;

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
  LCDWrapper(const pins_config::lcd_config &config);

  bool begin();
  void clear();
  void showConnection(bool show);
  void showPower(bool show);
  void setRow(uint8_t row, std::string_view text);
  std::string convertSecondsToHHMMSS(duration<uint16_t> duration) const;
  void update_chars(const BoardInfo &boardinfo);

private:
  static constexpr uint8_t CHAR_ANTENNA = 0;
  static constexpr uint8_t CHAR_CONNECTION = 1;
  static constexpr uint8_t CHAR_NO_CONNECTION = 2;
  static constexpr uint8_t CHAR_POWERED_ON = 3;
  static constexpr uint8_t CHAR_POWERED_OFF = 4;
  static constexpr uint8_t CHAR_POWERING_OFF = 5;
  // Character definitions
  static constexpr uint8_t antenna_char[8] = {0x15, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
  static constexpr uint8_t connection_char[8] = {0x00, 0x00, 0x01, 0x01, 0x05, 0x05, 0x15, 0x15};
  static constexpr uint8_t noconnection_char[8] = {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00};
  static constexpr uint8_t powered_on_char[8] = {0x04, 0x04, 0x04, 0x1f, 0x1f, 0x1f, 0x0a, 0x0a};
  static constexpr uint8_t powered_off_char[8] = {0x0a, 0x04, 0x0a, 0x00, 0x1f, 0x1f, 0x0a, 0x0a};
  static constexpr uint8_t powering_off_char[8] = {0x0e, 0x15, 0x15, 0x15, 0x17, 0x11, 0x11, 0x0e};

  const pins_config::lcd_config config;

  LiquidCrystal lcd;
  bool show_connection_status;
  bool show_power_status;
  bool forceUpdate;
  std::array<std::array<char, _COLS>, _ROWS> buffer;
  std::array<std::array<char, _COLS>, _ROWS> current;
  BoardInfo boardInfo;

  void backlightOn() const;
  void backlightOff() const;
  void prettyPrint(const std::array<std::array<char, _COLS>, _ROWS> &buffer, const BoardInfo &bi) const;
  bool needsUpdate(const BoardInfo &bi) const;

  void createChar(uint8_t char_idx, const uint8_t values[8]);
};

#include "LCDWrapper.tpp"
#endif // LCDWRAPPER_H_
