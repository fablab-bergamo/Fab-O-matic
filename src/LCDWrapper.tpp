#include <cstdint>
#include <string>
#include <array>
#include <sstream>
#include "LCDWrapper.hpp"

namespace fablabbg
{
  template <uint8_t _COLS, uint8_t _ROWS>
  LCDWrapper<_COLS, _ROWS>::LCDWrapper(const pins_config::lcd_config &conf) : config(conf),
                                                                              lcd(config.rs_pin, config.en_pin, config.d0_pin, config.d1_pin, config.d2_pin, config.d3_pin),
                                                                              show_connection_status(true), show_power_status(true), forceUpdate(true)
  {
    for (auto &row : this->buffer)
      row.fill({0});
    for (auto &row : this->current)
      row.fill({0});
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::createChar(uint8_t num, const uint8_t values[8])
  {
    // Arduino LCD library only reads uint8_t* but did not flag const, so we use this wrapper
    this->lcd.createChar(num, const_cast<uint8_t *>(values));
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  bool LCDWrapper<_COLS, _ROWS>::begin()
  {
    this->lcd.begin(_COLS, _ROWS);
    createChar(CHAR_ANTENNA, this->antenna_char);
    createChar(CHAR_CONNECTION, this->connection_char);
    createChar(CHAR_NO_CONNECTION, this->noconnection_char);
    createChar(CHAR_POWERED_OFF, this->powered_off_char);
    createChar(CHAR_POWERED_ON, this->powered_on_char);
    createChar(CHAR_POWERING_OFF, this->powering_off_char);

    if (this->config.bl_pin != NO_PIN)
      pinMode(this->config.bl_pin, OUTPUT);

    this->backlightOn();

    if (conf::debug::ENABLE_LOGS)
    {
      constexpr size_t MAX_LEN = 100;
      char buf[MAX_LEN] = {0};
      if (snprintf(buf, sizeof(buf), "Configured LCD %d x %d (d4=%d, d5=%d, d6=%d, d7=%d, en=%d, rs=%d), backlight=%d", _COLS, _ROWS,
                   this->config.d0_pin, this->config.d1_pin, this->config.d2_pin, this->config.d3_pin,
                   this->config.en_pin, this->config.rs_pin, this->config.bl_pin) > 0)
        Serial.println(buf);
    }

    return true;
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  std::string LCDWrapper<_COLS, _ROWS>::convertSecondsToHHMMSS(duration<uint16_t> duration) const
  {
    //! since something something does not support to_string we have to resort to ye olde cstring stuff
    char buf[9] = {0};

    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
             duration.count() / 3600UL,
             (duration.count() % 3600UL) / 60UL,
             duration.count() % 60UL);

    return {buf};
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::clear()
  {
    for (auto &row : this->current)
      row.fill({' '});
    this->lcd.clear();
    this->forceUpdate = true;
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::update(const BoardInfo &info, bool forced)
  {
    if (!forced && !this->needsUpdate(info))
    {
      return;
    }

    this->lcd.clear();

    auto row_num = 0;
    for (const auto &row : this->buffer)
    {
      this->lcd.setCursor(0, row_num);
      char why_arduino_has_not_implemented_liquidcrystal_from_char_array_yet[_COLS];
      memcpy(why_arduino_has_not_implemented_liquidcrystal_from_char_array_yet, &row, _COLS);
      this->lcd.print(why_arduino_has_not_implemented_liquidcrystal_from_char_array_yet);
      row_num++;
    }

    static_assert(_COLS > 1 && _ROWS > 1, "LCDWrapper required at least 2x2 LCDs");
    if (this->show_connection_status)
    {
      this->lcd.setCursor(_COLS - 2, 0);
      this->lcd.write(CHAR_ANTENNA);
      this->lcd.write(info.server_connected ? CHAR_CONNECTION : CHAR_NO_CONNECTION);
    }

    if (this->show_power_status)
    {
      this->lcd.setCursor(_COLS - 1, 1);
      if (info.power_state == Machine::PowerState::POWERED_ON)
      {
        this->lcd.write(CHAR_POWERED_ON);
      }
      else if (info.power_state == Machine::PowerState::POWERED_OFF)
      {
        this->lcd.write(CHAR_POWERED_OFF);
      }
      else if (info.power_state == Machine::PowerState::WAITING_FOR_POWER_OFF)
      {
        this->lcd.write(CHAR_POWERING_OFF);
      }
      else
      {
        this->lcd.write('?');
      }
    }

    this->current = this->buffer;
    this->boardInfo = info;
    this->forceUpdate = false;
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::showConnection(bool show)
  {
    if (this->show_connection_status != show)
      forceUpdate = true;

    this->show_connection_status = show;
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::showPower(bool show)
  {
    if (this->show_power_status != show)
      forceUpdate = true;

    this->show_power_status = show;
  }

  /// @brief Checks if the LCD buffer has changed since last write to the LCD, or forced update has been requested.
  /// @tparam _COLS number of columns
  /// @tparam _ROWS number of rows
  /// @param bi current state of the board
  /// @return true if the buffer has changed, false otherwise
  template <uint8_t _COLS, uint8_t _ROWS>
  bool LCDWrapper<_COLS, _ROWS>::needsUpdate(const BoardInfo &bi) const
  {
    if (forceUpdate || !(bi == this->boardInfo) || this->current != this->buffer)
    {
      if (conf::debug::ENABLE_LOGS)
      {
        this->prettyPrint(this->buffer, bi);
      }

      return true;
    }
    return false;
  }

  /// @brief Debug utility to print in the console the current buffer contents
  /// @tparam _COLS number of columns
  /// @tparam _ROWS number of rows
  /// @param buf character buffer
  /// @param bi board info, used for special indicators status
  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::prettyPrint(const DisplayBuffer &buf,
                                             const BoardInfo &bi) const
  {
    std::stringstream ss;
    ss << "/" << std::string(_COLS, '-') << "\\\r\n"; // LCD top

    for (const auto &row : buf)
    {
      ss << "|";
      for (const auto &ch : row)
      {
        if (ch == 0)
        {
          ss << " "; // Replace \0 with space
        }
        else
        {
          ss << ch;
        }
      }
      ss << "|\r\n"; // LCD right border
    }

    // LCD lower border
    ss << "\\" << std::string(_COLS, '-') << "/\r\n";

    auto str = ss.str();

    // Add symbols
    constexpr auto symbols_per_line = conf::lcd::COLS + 3;

    str[symbols_per_line * 2 - 2] = bi.server_connected ? 'Y' : 'x';

    if (bi.power_state == Machine::PowerState::POWERED_ON)
      str[symbols_per_line * 3 - 1] = 'Y';

    if (bi.power_state == Machine::PowerState::POWERED_OFF)
      str[symbols_per_line * 3 - 1] = 'x';

    if (bi.power_state == Machine::PowerState::WAITING_FOR_POWER_OFF)
      str[symbols_per_line * 3 - 1] = '!';

    Serial.print(str.c_str());
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::setRow(uint8_t row, std::string_view text)
  {
    if (row < _ROWS)
    {
      this->buffer[row].fill({0});
      for (auto i = 0; i < text.length() && i < _COLS; i++)
      {
        this->buffer[row][i] = text[i];
      }
    }
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::backlightOn() const
  {
    if (this->config.bl_pin != NO_PIN)
      digitalWrite(this->config.bl_pin, this->config.active_low ? 0 : 1);
  }

  template <uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<_COLS, _ROWS>::backlightOff() const
  {
    if (this->config.bl_pin != NO_PIN)
      digitalWrite(this->config.bl_pin, this->config.active_low ? 1 : 0);
  }
} // namespace fablabbg