#include <cstdint>
#include <string>
#include <array>

#include "BoardState.h"
#include "LCDWrapper.h"
#include "Machine.h"

template <uint8_t _COLS, uint8_t _ROWS>
LCDWrapper<_COLS, _ROWS>::LCDWrapper(Config config) : config(config),
                                                      lcd(config.rs, config.enable, config.d0, config.d1, config.d2, config.d3),
                                                      show_connection_status(false), show_power_status(true), forceUpdate(true)
{
  buffer.fill({0});
  current.fill({0});
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::createChar(uint8_t num, const uint8_t values[8])
{
  // LCD library only reads uint8_t* but did not flag const
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

  if (this->config.backlight_pin != NO_PIN)
    pinMode(this->config.backlight_pin, OUTPUT);

  this->backlightOn();

  char buffer[80] = {0};
  sprintf(buffer, "Configured LCD %d x %d (d4=%d, d5=%d, d6=%d, d7=%d, en=%d, rs=%d), backlight=%d", _COLS, _ROWS, this->config.d0, this->config.d1, this->config.d2, this->config.d3, this->config.enable, this->config.rs, this->config.backlight_pin);
  Serial.println(buffer);
  return true;
}

template <uint8_t _COLS, uint8_t _ROWS>
std::string LCDWrapper<_COLS, _ROWS>::convertSecondsToHHMMSS(unsigned long milliseconds) const
{
  //! since something something does not support to_string we have to resort to ye olde cstring stuff
  char buffer[9];
  unsigned long seconds = milliseconds / 1000;
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", (int)(seconds / 3600), (int)((seconds % 3600) / 60), (int)(seconds % 60));

  return {buffer};
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::clear()
{
  this->current.fill({0});
  this->lcd.clear();
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::update_chars(const BoardInfo &info)
{
  if (this->needsUpdate(info))
  {
    this->lcd.clear();
    this->lcd.setCursor(0, 0);

    char why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet[16];
    memcpy(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet, &this->buffer[0], _COLS);

    this->lcd.print(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet);

    if (this->show_connection_status)
    {
      this->lcd.setCursor(14, 0);
      this->lcd.write(CHAR_ANTENNA);
      this->lcd.write(info.server_connected ? CHAR_CONNECTION : CHAR_NO_CONNECTION);
    }

    this->lcd.setCursor(0, 1);
    memcpy(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet, &this->buffer[1], _COLS);
    this->lcd.print(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet);

    if (this->show_power_status)
    {
      this->lcd.setCursor(15, 1);
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

template <uint8_t _COLS, uint8_t _ROWS>
bool LCDWrapper<_COLS, _ROWS>::needsUpdate(const BoardInfo &bi) const
{
  if (this->current != this->buffer || !(bi == this->boardInfo) || forceUpdate)
  {
    this->prettyPrint(this->buffer);
    return true;
  }
  return false;
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::prettyPrint(const std::array<std::array<char, _COLS>, _ROWS> &buffer) const
{
  // LCD upper border
  Serial.print("/");
  for (auto i = 0; i < _COLS; i++)
    Serial.print("-");
  Serial.println("\\");
  
  for (auto i = 0; i < _ROWS; i++)
  {
    Serial.print("|"); // LCD left border
    for (auto j = 0; j < _COLS; j++)
    {
      if (this->buffer[i][j] == 0)
      {
        Serial.print(' '); // Replace \0 with space
      }
      else
      {
        Serial.print(this->buffer[i][j]);
      }
    }
    Serial.println("|"); // LCD right border
  }

  // LCD lower border
  Serial.print("\\");
  for (auto i = 0; i < _COLS; i++)
    Serial.print("-");
  Serial.println("/");
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::setRow(uint8_t row, const std::string_view text)
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
  if (this->config.backlight_pin != NO_PIN)
    digitalWrite(this->config.backlight_pin, this->config.backlight_active_low ? 0 : 1);
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::backlightOff() const
{
  if (this->config.backlight_pin != NO_PIN)
    digitalWrite(this->config.backlight_pin, this->config.backlight_active_low ? 1 : 0);
}
