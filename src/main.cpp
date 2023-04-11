#include <cstdint>
#include <string>
#include <array>
#include <chrono>

#include <LiquidCrystal.h>
#include <WiFi.h>

#include "Board.h"
#include "BoardState.h"
#include "pins.h"

static bool ready_for_a_new_card = true;
static BoardState board;
static u_int16_t no_card_cpt = 0;
static unsigned long last_server_poll = 1;

/// @brief connects and polls the server for up-to-date machine information
void refreshFromServer()
{
  if (Board::server.connect())
  {
    // Check the configured machine data from the server
    auto result = Board::server.checkMachine(secrets::machine::machine_id);
    if (result.request_ok)
    {
      if (result.is_valid)
      {
        Serial.printf("The configured machine ID %d is valid, maintenance=%d, allowed=%d\n", secrets::machine::machine_id, result.needs_maintenance, result.allowed);
        Board::machine.maintenanceNeeded = result.needs_maintenance;
        Board::machine.allowed = result.allowed;
      }
      else
      {
        Serial.printf("The configured machine ID %d is unknown to the server\n", secrets::machine::machine_id);
      }
    }
  }
}

bool tryConnect()
{
  Serial.println("Trying Wifi and server connection...");
  // connection to wifi
  board.changeStatus(BoardState::Status::CONNECTING);

  // Try to connect
  Board::server.connect();

  // Get machine data from the server if it is online
  refreshFromServer();

  // Refresh after connection
  board.changeStatus(Board::server.isOnline() ? BoardState::Status::CONNECTED : BoardState::Status::OFFLINE);
  delay(500);
  return Board::server.isOnline();
}

void setup()
{
  Serial.begin(115200); // Initialize serial communications with the PC for debugging.
  Serial.println("Starting setup!");
  delay(100);
  board.init();
  delay(100);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  delay(1000);
}

void loop()
{
  delay(100);

  // Regular connection management
  if (last_server_poll == 0)
  {
    last_server_poll = millis();
  }
  if (millis() - last_server_poll >  conf::server::REFRESH_PERIOD_SECONDS * 1000)
  {
    Serial.printf("Free heap:%d bytes\n", ESP.getFreeHeap());
    if (Board::server.isOnline())
    {
      // Get machine data from the server
      refreshFromServer();
    }
    else
    {
      // Reconnect
      tryConnect();
    }
    last_server_poll = 0;
  }

  // check if there is a card
  if (Board::rfid.IsNewCardPresent())
  {
    Serial.println("New card present");
    // if there is a "new" card (could be the same that stayed in the field)
    if (!Board::rfid.ReadCardSerial() || !ready_for_a_new_card)
    {
      return;
    }
    no_card_cpt = 0;
    ready_for_a_new_card = false;

    // Acquire the UID of the card
    FabUser candidate = Board::rfid.GetUser();
    if (Board::machine.isFree())
    {
      // machine is free
      if (board.authorize(candidate.member_uid))
      {
        Serial.println("Login successfull");
      }
      else
      {
        Serial.println("Login failed");
      }
      delay(1000);
    }
    else
    {
      // machine is busy
      if (Board::machine.getActiveUser() == candidate)
      {
        // we can logout. we should require that the card stays in the field for some seconds, to prevent accidental logout. maybe sound a buzzer?
        board.logout();
        delay(1000);
      }
      else
      {
        // user is not the same, display who is using it
        board.changeStatus(BoardState::Status::ALREADY_IN_USE);
        delay(1000);
      }
    }
  }
  else
  {
    no_card_cpt++;
    if (no_card_cpt > 10) // we wait for get SOME "no card" before flipping this flag
      ready_for_a_new_card = true;

    if (Board::machine.isFree())
    {
      board.changeStatus(BoardState::Status::FREE);

      if (Board::machine.shutdownWarning())
      {
        // TODO : beep
      }
      if (Board::machine.canPowerOff())
      {
        Board::machine.power(false);
      }
    }
    else
    {
      board.changeStatus(BoardState::Status::IN_USE);

      // auto logout after delay
      if (conf::machine::TIMEOUT_USAGE_MINUTES > 0 &&
          Board::machine.getUsageTime() > conf::machine::TIMEOUT_USAGE_MINUTES * 60 * 1000)
      {
        Serial.println("Auto-logging out user");
        board.logout();
        delay(1000);
      }
    }
  }
}