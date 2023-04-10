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

void tryConnect()
{
  Serial.println("Trying Wifi and server connection...");
  // connection to wifi
  board.changeStatus(BoardState::Status::CONNECTING);
  
  if (Board::server.connect()) 
  {
    Serial.println("Connection successfull");
  }
  // Get machine data from the server
  refreshFromServer();

  // Refresh after connection
  board.changeStatus(Board::server.isOnline() ? BoardState::Status::CONNECTED : BoardState::Status::OFFLINE);
  delay(500);
}

void setup()
{
  Serial.begin(115200); // Initialize serial communications with the PC for debugging.
  Serial.println("Starting setup!");
  delay(100);
  board.init();
  delay(100);
  tryConnect();
}

void loop()
{
  delay(100);

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
      }
      else
      {
        // user is not the same, display who is using it
        board.changeStatus(BoardState::Status::ALREADY_IN_USE);
        delay(1000);
      }
      delay(1000);
    }
  }
  else
  {
    no_card_cpt++;
    if (no_card_cpt > 10) // we wait for get SOME "no card" before flipping this flag
      ready_for_a_new_card = true;

    if (!Board::machine.isFree())
    {
      board.changeStatus(BoardState::Status::IN_USE);
    }
    else
    {
      board.changeStatus(BoardState::Status::FREE);
    }
  }
}