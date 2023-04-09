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

void setup()
{
  Serial.begin(115200); // Initialize serial communications with the PC for debugging.
  Serial.println("Starting setup!");
  delay(100);
  board.init();
  Serial.println("Trying Wifi connection...");
  // connection to wifi
  board.changeStatus(BoardState::Status::CONNECTING);
  // TODO : connect
  delay(1000);
  // Refresh after connection
  board.changeStatus(Board::server.isOnline() ? BoardState::Status::CONNECTED : BoardState::Status::OFFLINE);
  delay(1000);
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
    ready_for_a_new_card = true; // we should get SOME "no card" before flipping this flag
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