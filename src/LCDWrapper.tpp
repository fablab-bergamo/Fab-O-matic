#include <cstdint>
#include <string>
#include <array>
#include "LCDWrapper.h"

template <uint8_t _COLS, uint8_t _ROWS>
LCDWrapper<_COLS, _ROWS>::LCDWrapper(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) : _lcd(rs, enable, d0, d1, d2, d3)
{
  _backlight_pin = 255;
  _backlight_active_low = false;
  _show_connection_status = false;
  _connection_status = false;
  _buffer.fill({0});
  _current.fill({0});
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::begin()
{
  this->_lcd.begin(_ROWS, _COLS);
  this->_lcd.createChar(0, _antenna_char);
  this->_lcd.createChar(1, _connection_char);
  this->_lcd.createChar(2, _noconnection_char);
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::update(FabServer server, FabMember user)
{
  switch (_state)
  {
  /* TODO */
  case LCDState::CLEAR:
    this->clear();
    break;
  case LCDState::FREE:
    this->setRow(0, server.isOnline() ? "Disponibile" : "OFFLINE");
    this->setRow(1, "Avvicina carta");
    break;
  case LCDState::LOGGED_IN:
    this->setRow(0, "Inizio uso");
    this->setRow(1, user.getName());
    break;
  case LCDState::LOGIN_DENIED:
    this->setRow(0, "Negato");
    this->setRow(1, "Carta sconosciuta");
    break;
  case LCDState::LOGOUT:
    this->setRow(0, "Arrivederci");
    this->setRow(1, user.getName());
    break;
  case LCDState::CONNECTING:
    this->setRow(0, "Connecting");
    break;
  case LCDState::CONNECTED:
    this->setRow(0, "Connected");
    break;
  case LCDState::BUSY:
    this->setRow(0, "Busy");
    break;
  }
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::clear()
{
  this->_lcd.clear();
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::state(LCDState::LCDStateType state)
{
  this->_state = state;
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::_update_chars()
{
  if (this->_needsUpdate())
  {
    this->_lcd.clear();
    this->_lcd.setCursor(0, 0);

    char why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet[16];
    for (auto i = 0; i < _COLS; i++)
    {
      why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet[i] = this->_buffer[0][i];
    }
    this->_lcd.print(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet);

    if (this->_show_connection_status)
    {
      this->_lcd.setCursor(14, 0);
      this->_lcd.write((uint8_t)0);
      this->_lcd.write(this->_connection_status ? (uint8_t)1 : (uint8_t)2);
    }

    this->_lcd.setCursor(0, 1);
    for (auto i = 0; i < _COLS; i++)
    {
      why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet[i] = this->_buffer[1][i];
    }
    this->_lcd.print(why_arduino_has_not_implemented_liquidcrystal_print_from_char_array_yet);

    _current = _buffer;
  }
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::setConnectionState(bool connected)
{
  this->_connection_status = connected;
}

template <uint8_t _COLS, uint8_t _ROWS>
void LCDWrapper<_COLS, _ROWS>::showConnection(bool show)
{
  this->_show_connection_status = show;
}

template <uint8_t _COLS, uint8_t _ROWS>
bool LCDWrapper<_COLS, _ROWS>::_needsUpdate()
{
  if (this->_current != this->_buffer)
  {

    Serial.println("buffer dump:");
    for (auto i = 0; i < _ROWS; i++)
    {
      for (auto j = 0; j < _COLS; j++)
      {
        Serial.print(this->_buffer[i][j]);
      }
      Serial.println();
      for (auto j = 0; j < _COLS; j++)
      {
        Serial.print(this->_current[i][j]);
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
    this->_buffer[row].fill({0});
    for (auto i = 0; i < _COLS; i++)
    {
      this->_buffer[row][i] = text[i];
    }
  }
}
