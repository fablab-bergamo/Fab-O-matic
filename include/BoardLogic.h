#ifndef BOARDLOGIC_H
#define BOARDLOGIC_H

#include "FabUser.h"
#include "card.h"
#include <Adafruit_NeoPixel.h>
#include "pins.h"

class BoardLogic
{
public:
  enum class Status
  {
    CLEAR,
    FREE,
    LOGGED_IN,
    LOGIN_DENIED,
    BUSY,
    LOGOUT,
    CONNECTING,
    CONNECTED,
    ALREADY_IN_USE,
    IN_USE,
    OFFLINE,
    NOT_ALLOWED,
    VERIFYING,
    MAINTENANCE_NEEDED,
    MAINTENANCE_QUERY,
    MAINTENANCE_DONE,
    ERROR
  };

  BoardLogic() noexcept;

  Status getStatus() const;
  FabUser getUser();
  Adafruit_NeoPixel pixels{1, pins.led.pin, NEO_GRB + NEO_KHZ800};

  void refreshFromServer();
  void onNewCard();
  void logout();
  bool authorize(const card::uid_t uid);
  void changeStatus(Status newStatus);
  bool init();
  void updateLCD() const;
  void beep_ok() const;
  void beep_failed() const;
  void led(bool value);
  void invert_led();
  void set_led_color(uint8_t r, uint8_t g, uint8_t b);

  bool ready_for_a_new_card = true;
  bool led_status = false;

  // copy reference
  BoardLogic &operator=(const BoardLogic &board) = delete;
  // copy constructor
  BoardLogic(const BoardLogic &) = delete;
  // move constructor
  BoardLogic(BoardLogic &&) = default;
  // move assignment
  BoardLogic &operator=(BoardLogic &&) = default;

private:
  Status status;
  FabUser user;
  uint8_t led_color[3] = {0, 255, 0};

  bool longTap(std::string_view short_prompt) const;
};

#endif // BOARDLOGIC_H