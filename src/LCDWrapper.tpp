#include <cstdint>
#include <string>
#include <array>

#include "BoardState.h"
#include "LCDWrapper.h"

template <uint8_t _COLS, uint8_t _ROWS>
LCDWrapper<_COLS, _ROWS>::LCDWrapper(Config config) : lcd(config.rs, config.enable, config.d0, config.d1, config.d2, config.d3),
                                                      backlight_pin(config.backlight_pin),
                                                      backlight_active_low(config.backlight_active_low),
                                                      show_connection_status(false),
                                                      connection_status(false)
{
  buffer.fill({0});
  current.fill({0});
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::begin()
{
  this->lcd.begin(_COLS, _ROWS);
  this->lcd.createChar(0, this->antenna_char);
  this->lcd.createChar(1, this->connection_char);
  this->lcd.createChar(2, this->noconnection_char);
}

template <uint8_t _COLS, uint8_t _ROWS>
std::string LCDWrapper<_COLS, _ROWS>::convertSecondsToHHMMSS(unsigned long milliseconds)
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
  this->lcd.clear();
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::update_chars()
{
  if (this->needsUpdate())
  {
    this->lcd.clear();
    this->lcd.setCursor(0, 0);

    char why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet[16];
    memcpy(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet, &this->buffer[0], _COLS);

    this->lcd.print(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet);

    if (this->show_connection_status)
    {
      this->lcd.setCursor(14, 0);
      this->lcd.write((uint8_t)0);
      this->lcd.write(this->connection_status ? (uint8_t)1 : (uint8_t)2);
    }

    this->lcd.setCursor(0, 1);
    memcpy(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet, &this->buffer[1], _COLS);
    this->lcd.print(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet);

    current = buffer;
  }
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::setConnectionState(bool connected)
{
  this->connection_status = connected;
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::showConnection(bool show)
{
  this->show_connection_status = show;
}

template <uint8_t _COLS, uint8_t _ROWS>
bool LCDWrapper<_COLS, _ROWS>::needsUpdate()
{
  if (this->current != this->buffer)
  {
    Serial.println("buffer dump:");
    for (auto i = 0; i < _ROWS; i++)
    {
      for (auto j = 0; j < _COLS; j++)
      {
        Serial.print(this->buffer[i][j]);
      }
      Serial.println();
      for (auto j = 0; j < _COLS; j++)
      {
        Serial.print(this->current[i][j]);
      }
      Serial.println();
      Serial.println();
    }

    return true;
  }
  return false;
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::setRow(uint8_t row, std::string text)
{
  if (row < _ROWS)
  {
    this->buffer[row].fill({0});
    for (auto i = 0; i < text.length(); i++)
    {
      this->buffer[row][i] = text[i];
    }
  }
}
