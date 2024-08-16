#include "LCDWrapper.hpp"
#include "Logging.hpp"
#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>

namespace fabomatic
{

  LCDWrapper::LCDWrapper(const pins_config::lcd_config &conf) : config(conf),
                                                                lcd(config.rs_pin, config.en_pin, config.d0_pin, config.d1_pin, config.d2_pin, config.d3_pin),
                                                                show_connection_status(true), show_power_status(true), forceUpdate(true)
  {
    digitalWrite(config.rs_pin, LOW);
    pinMode(config.rs_pin, OUTPUT);
    digitalWrite(config.en_pin, HIGH);
    pinMode(config.en_pin, OUTPUT);
    digitalWrite(config.d0_pin, LOW);
    pinMode(config.d0_pin, OUTPUT);
    digitalWrite(config.d1_pin, LOW);
    pinMode(config.d1_pin, OUTPUT);
    digitalWrite(config.d2_pin, LOW);
    pinMode(config.d2_pin, OUTPUT);
    digitalWrite(config.d3_pin, LOW);
    pinMode(config.d3_pin, OUTPUT);

    for (auto &row : buffer)
      row.fill({0});
    for (auto &row : current)
      row.fill({0});
  }

  bool LCDWrapper::begin()
  {
    lcd.begin(conf::lcd::COLS, conf::lcd::ROWS);

    lcd.createChar(CHAR_ANTENNA, antenna_char.data());
    lcd.createChar(CHAR_CONNECTION, connection_char.data());
    lcd.createChar(CHAR_NO_CONNECTION, noconnection_char.data());
    lcd.createChar(CHAR_POWERED_OFF, powered_off_char.data());
    lcd.createChar(CHAR_POWERED_ON, powered_on_char.data());
    lcd.createChar(CHAR_POWERING_OFF, powering_off_char.data());

    if (config.bl_pin != NO_PIN)
      pinMode(config.bl_pin, OUTPUT);

    backlightOn();

    ESP_LOGI(TAG, "Configured LCD %d x %d (d4=%d, d5=%d, d6=%d, d7=%d, en=%d, rs=%d), backlight=%d", conf::lcd::COLS, conf::lcd::ROWS,
             config.d0_pin, config.d1_pin, config.d2_pin, config.d3_pin,
             config.en_pin, config.rs_pin, config.bl_pin);

    return true;
  }

  void LCDWrapper::clear()
  {
    for (auto &row : current)
      row.fill({' '});
    lcd.clear();
    forceUpdate = true;
  }

  void LCDWrapper::update(const BoardInfo &info, bool forced)
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
      lcd.print(row.data());
      row_num++;
    }

    static_assert(conf::lcd::COLS > 1 && conf::lcd::ROWS > 1, "LCDWrapper required at least 2x2 LCDs");
    if (show_connection_status)
    {
      lcd.setCursor(conf::lcd::COLS - 2, 0);
      lcd.write(CHAR_ANTENNA);
      if (info.mqtt_connected)
      {
        lcd.write(CHAR_CONNECTION);
      }
      else if (info.unresponsive)
      {
        lcd.write('?');
      }
      else
      {
        lcd.write(CHAR_NO_CONNECTION);
      }
    }

    if (show_power_status)
    {
      lcd.setCursor(conf::lcd::COLS - 1, 1);
      if (info.power_state == Machine::PowerState::PoweredOn)
      {
        lcd.write(CHAR_POWERED_ON);
      }
      else if (info.power_state == Machine::PowerState::PoweredOff)
      {
        lcd.write(CHAR_POWERED_OFF);
      }
      else if (info.power_state == Machine::PowerState::WaitingPowerOff)
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

  void LCDWrapper::showConnection(bool show)
  {
    if (show_connection_status != show)
      forceUpdate = true;

    show_connection_status = show;
  }

  void LCDWrapper::showPower(bool show)
  {
    if (show_power_status != show)
      forceUpdate = true;

    show_power_status = show;
  }

  /// @brief Checks if the LCD buffer has changed since last write to the LCD, or forced update has been requested.
  /// @param bi current state of the board
  /// @return true if the buffer has changed, false otherwise

  bool LCDWrapper::needsUpdate(const BoardInfo &bi) const
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
  /// @param buf character buffer
  /// @param bi board info, used for special indicators status

  void LCDWrapper::prettyPrint(const DisplayBuffer &buf,
                               const BoardInfo &bi) const
  {
    std::stringstream ss{};
    ss << "/" << std::string(conf::lcd::COLS, '-') << "\\\r\n"; // LCD top

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
    ss << "\\" << std::string(conf::lcd::COLS, '-') << "/\r\n";

    auto str = ss.str();

    // Add symbols
    constexpr auto symbols_per_line = conf::lcd::COLS + 3;

    if (bi.mqtt_connected)
    {
      str[symbols_per_line * 2 - 2] = 'Y';
    }
    else if (bi.unresponsive)
    {
      str[symbols_per_line * 2 - 2] = '?';
    }
    else
    {
      str[symbols_per_line * 2 - 2] = 'x';
    }

    if (bi.power_state == Machine::PowerState::PoweredOn)
      str[symbols_per_line * 3 - 1] = 'Y';

    if (bi.power_state == Machine::PowerState::PoweredOff)
      str[symbols_per_line * 3 - 1] = 'x';

    if (bi.power_state == Machine::PowerState::WaitingPowerOff)
      str[symbols_per_line * 3 - 1] = '!';

    Serial.print(str.c_str());
  }

  void LCDWrapper::setRow(uint8_t row, const std::string_view &text)
  {
    if (row < conf::lcd::ROWS)
    {
      buffer[row].fill({0});
      for (auto i = 0; i < text.length() && i < conf::lcd::COLS; i++)
      {
        buffer[row][i] = text[i];
      }
    }
  }

  void LCDWrapper::backlightOn() const
  {
    if (config.bl_pin != NO_PIN)
      digitalWrite(config.bl_pin, config.active_low ? LOW : HIGH);
  }

  void LCDWrapper::backlightOff() const
  {
    if (config.bl_pin != NO_PIN)
      digitalWrite(config.bl_pin, config.active_low ? HIGH : LOW);
  }

  auto LCDWrapper::convertSecondsToHHMMSS(std::chrono::seconds duration) const -> const std::string
  {
    std::stringstream ss{};
    const auto hrs = std::chrono::duration_cast<std::chrono::hours>(duration);
    const auto mins = std::chrono::duration_cast<std::chrono::minutes>(duration - hrs);
    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration - hrs - mins);

    ss << std::setfill('0') << std::setw(2) << hrs.count() << ":"
       << std::setfill('0') << std::setw(2) << mins.count() << ":"
       << std::setfill('0') << std::setw(2) << secs.count();

    return ss.str();
  }
} // namespace fabomatic