#include <cstdint>
#include <string>
#include <array>
#include <sstream>
#include "LCDWrapper.hpp"
#include <type_traits>

namespace fablabbg
{
  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  LCDWrapper<TLcd, _COLS, _ROWS>::LCDWrapper(const pins_config::lcd_config &conf) : config(conf),
                                                                                    lcd(config.rs_pin, config.en_pin, config.d0_pin, config.d1_pin, config.d2_pin, config.d3_pin),
                                                                                    show_connection_status(true), show_power_status(true), forceUpdate(true)
  {
    for (auto &row : buffer)
      row.fill({0});
    for (auto &row : current)
      row.fill({0});
  }

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::createChar(uint8_t num, const uint8_t values[8])
  {
    // Arduino LCD library only reads uint8_t* but did not flag const, so we use this wrapper
    lcd.createChar(num, const_cast<uint8_t *>(values));
  }

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  bool LCDWrapper<TLcd, _COLS, _ROWS>::begin()
  {
    lcd.begin(_COLS, _ROWS);
    createChar(CHAR_ANTENNA, antenna_char);
    createChar(CHAR_CONNECTION, connection_char);
    createChar(CHAR_NO_CONNECTION, noconnection_char);
    createChar(CHAR_POWERED_OFF, powered_off_char);
    createChar(CHAR_POWERED_ON, powered_on_char);
    createChar(CHAR_POWERING_OFF, powering_off_char);

    if (config.bl_pin != NO_PIN)
      pinMode(config.bl_pin, OUTPUT);

    backlightOn();

    if (conf::debug::ENABLE_LOGS)
    {
      constexpr size_t MAX_LEN = 100;
      char buf[MAX_LEN] = {0};
      if (snprintf(buf, sizeof(buf), "Configured LCD %d x %d (d4=%d, d5=%d, d6=%d, d7=%d, en=%d, rs=%d), backlight=%d", _COLS, _ROWS,
                   config.d0_pin, config.d1_pin, config.d2_pin, config.d3_pin,
                   config.en_pin, config.rs_pin, config.bl_pin) > 0)
        Serial.println(buf);
    }

    return true;
  }

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::clear()
  {
    for (auto &row : current)
      row.fill({' '});
    lcd.clear();
    forceUpdate = true;
  }

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::update(const BoardInfo &info, bool forced)
  {
    if (!forced && !needsUpdate(info))
    {
      return;
    }

    lcd.clear();

    auto row_num = 0;
    for (const auto &row : buffer)
    {
      lcd.setCursor(0, row_num);
      char why_arduino_has_not_implemented_liquidcrystal_from_char_array_yet[_COLS];
      memcpy(why_arduino_has_not_implemented_liquidcrystal_from_char_array_yet, &row, _COLS);
      lcd.print(why_arduino_has_not_implemented_liquidcrystal_from_char_array_yet);
      row_num++;
    }

    static_assert(_COLS > 1 && _ROWS > 1, "LCDWrapper required at least 2x2 LCDs");
    if (show_connection_status)
    {
      lcd.setCursor(_COLS - 2, 0);
      lcd.write(CHAR_ANTENNA);
      lcd.write(info.server_connected ? CHAR_CONNECTION : CHAR_NO_CONNECTION);
    }

    if (show_power_status)
    {
      lcd.setCursor(_COLS - 1, 1);
      if (info.power_state == Machine::PowerState::POWERED_ON)
      {
        lcd.write(CHAR_POWERED_ON);
      }
      else if (info.power_state == Machine::PowerState::POWERED_OFF)
      {
        lcd.write(CHAR_POWERED_OFF);
      }
      else if (info.power_state == Machine::PowerState::WAITING_FOR_POWER_OFF)
      {
        lcd.write(CHAR_POWERING_OFF);
      }
      else
      {
        lcd.write('?');
      }
    }

    current = buffer;
    boardInfo = info;
    forceUpdate = false;
  }

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::showConnection(bool show)
  {
    if (show_connection_status != show)
      forceUpdate = true;

    show_connection_status = show;
  }

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::showPower(bool show)
  {
    if (show_power_status != show)
      forceUpdate = true;

    show_power_status = show;
  }

  /// @brief Checks if the LCD buffer has changed since last write to the LCD, or forced update has been requested.
  /// @tparam _COLS number of columns
  /// @tparam _ROWS number of rows
  /// @param bi current state of the board
  /// @return true if the buffer has changed, false otherwise
  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  bool LCDWrapper<TLcd, _COLS, _ROWS>::needsUpdate(const BoardInfo &bi) const
  {
    if (forceUpdate || !(bi == boardInfo) || current != buffer)
    {
      if (conf::debug::ENABLE_LOGS)
      {
        prettyPrint(buffer, bi);
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
  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::prettyPrint(const DisplayBuffer &buf,
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

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::setRow(uint8_t row, const std::string_view text)
  {
    if (text.length() >= _COLS)
      Serial.printf("LCDWrapper::setRow: text too long : %s\r\n", text.data());

    if (row < _ROWS)
    {
      buffer[row].fill({0});
      for (auto i = 0; i < text.length() && i < _COLS; i++)
      {
        buffer[row][i] = text[i];
      }
    }
  }

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::backlightOn() const
  {
    if (config.bl_pin != NO_PIN)
      digitalWrite(config.bl_pin, config.active_low ? 0 : 1);
  }

  template <typename TLcd, uint8_t _COLS, uint8_t _ROWS>
  void LCDWrapper<TLcd, _COLS, _ROWS>::backlightOff() const
  {
    if (config.bl_pin != NO_PIN)
      digitalWrite(config.bl_pin, config.active_low ? 1 : 0);
  }
} // namespace fablabbg