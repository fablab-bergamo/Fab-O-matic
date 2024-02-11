#ifndef BOARDLOGIC_HPP_
#define BOARDLOGIC_HPP_

#include "AuthProvider.hpp"
#include "BaseLCDWrapper.hpp"
#include "BaseRfidWrapper.hpp"
#include "FabBackend.hpp"
#include "FabUser.hpp"
#include "Led.hpp"
#include "Machine.hpp"
#include "card.hpp"
#include "pins.hpp"
#include "secrets.hpp"
#include <Adafruit_NeoPixel.h>

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
      PORTAL_STARTING,
      BOOT,
      SHUTDOWN_IMMINENT,
      OTA_START,
      FACTORY_RESET,
    };

    BoardLogic() = default;

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

    bool configure(BaseRFIDWrapper &rfid, BaseLCDWrapper &lcd);

    void blinkLed(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
    void checkRfid();
    void checkPowerOff();
    void setAutologoffDelay(std::chrono::seconds delay);
    void setWhitelist(WhiteList whitelist);
    FabBackend &getServer();
    bool reconfigure();

    Machine &getMachineForTesting();
    const Machine &getMachine() const;

    bool rebootRequest{false};

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
    Led led;
    FabBackend server;
    std::optional<std::reference_wrapper<BaseRFIDWrapper>> rfid{std::nullopt}; // Configured at runtime
    std::optional<std::reference_wrapper<BaseLCDWrapper>> lcd{std::nullopt};   // Configured at runtime
    bool ready_for_a_new_card{true};
    bool led_status{false};

    Machine machine;
    AuthProvider auth{secrets::cards::whitelist};

    BaseRFIDWrapper &getRfid() const;
    BaseLCDWrapper &getLcd() const;

    bool longTap(const card::uid_t card, const std::string &short_prompt) const;
  };
} // namespace fablabbg
#endif // BOARDLOGIC_HPP_