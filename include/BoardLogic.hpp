#ifndef BOARDLOGIC_H
#define BOARDLOGIC_H

#include "FabUser.hpp"
#include "card.hpp"
#include <Adafruit_NeoPixel.h>
#include "pins.hpp"
#include "FabServer.hpp"
#include "BaseRfidWrapper.hpp"
#include "AuthProvider.hpp"
#include "Machine.hpp"
#include "secrets.hpp"
#include "BaseLCDWrapper.hpp"
#include "BaseRfidWrapper.hpp"

namespace fablabbg
{
  class BoardLogic
  {
  public:
    enum class Status : uint8_t
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
      ERROR,
      ERROR_HW,
      PORTAL_FAILED,
      PORTAL_OK,
      PORTAL_STARTING
    };

    BoardLogic();

    Status getStatus() const;

    void refreshFromServer();
    void onNewCard(card::uid_t uid);
    void logout();
    bool authorize(const card::uid_t uid);
    void changeStatus(Status newStatus);
    bool board_init();
    void updateLCD() const;
    void beep_ok() const;
    void beep_failed() const;
    void led(bool value);
    void invert_led();
    void set_led_color(uint8_t r, uint8_t g, uint8_t b);
    void shortDelay(); // Waits for some time to allow the user to read the LCD screen

    bool configure(BaseRFIDWrapper &rfid, BaseLCDWrapper &lcd);

    void refreshLCD() const;
    void blinkLed();
    void checkRfid();
    void checkPowerOff();
    void setAutologoffDelay(seconds delay);
    void setWhitelist(WhiteList whitelist);
    FabServer &getServer() const;
    bool reconfigure();

    Machine &getMachineForTesting();
    const Machine &getMachine() const;

    // copy reference
    BoardLogic &operator=(const BoardLogic &board) = delete;
    // copy constructor
    BoardLogic(const BoardLogic &) = delete;
    // move constructor
    BoardLogic(BoardLogic &&) = default;
    // move assignment
    BoardLogic &operator=(BoardLogic &&) = default;

  private:
    Status status{Status::CLEAR};
    uint8_t led_color[3] = {0, 255, 0};
    Adafruit_NeoPixel pixels{1, pins.led.pin, NEO_GRB + NEO_KHZ800};
    std::unique_ptr<FabServer> server;
    std::optional<std::reference_wrapper<BaseRFIDWrapper>> rfid{std::nullopt};
    std::optional<std::reference_wrapper<BaseLCDWrapper>> lcd{std::nullopt};
    bool ready_for_a_new_card{true};
    bool led_status{false};

    mutable Machine machine;
    mutable AuthProvider auth{secrets::cards::whitelist};

    BaseRFIDWrapper &getRfid() const;
    BaseLCDWrapper &getLcd() const;

    bool longTap(const card::uid_t card, const std::string &short_prompt) const;
  };
} // namespace fablabbg
#endif // BOARDLOGIC_H