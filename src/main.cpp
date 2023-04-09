#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"

#include "pins.h"
#include "secrets.h"
#include "Machine.h"
#include "FabServer.h"
#include "LCDWrapper.h"
#include "BoardState.h"

#include <cstdint>
#include <string>
#include <array>
#include <chrono>

#include <LiquidCrystal.h>
#include <WiFi.h>
#include "conf.h"

static bool ready_for_a_new_card = true;
static Board::BoardState board;

void setup()
{
  Serial.begin(115200); // Initialize serial communications with the PC for debugging.
  Serial.println("Starting setup!");
  delay(100);
  Serial.println("Initializing SPI");
  SPI.begin(pins::mfrc522::sck_pin, pins::mfrc522::miso_pin, pins::mfrc522::mosi_pin, pins::mfrc522::cs_pin);
  delay(100);

  board.init();

  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  
  // connection to wifi
  board.changeStatus(BoardStatus::CONNECTING);
  
  board.getLCD().showConnection(true);
  board.getLCD().setConnectionState(board.getServer().isOnline());

  board.changeStatus(board.getServer().isOnline() ? BoardStatus::CONNECTED : BoardStatus::OFFLINE);
  delay(1000);
}

void loop()
{
  delay(100);

  // check if there is a card
  if (board.getRfid().IsNewCardPresent())
  {
    // if there is a "new" card (could be the same that stayed in the field)
    if (!board.getRfid().ReadCardSerial() || !ready_for_a_new_card)
    {
      return;
    }
    ready_for_a_new_card = false;

    // Acquire the UID of the card
    byte uid[10];
    board.getRfid().SetUid(uid);

    if (board.getMachine().isFree())
    {
      // machine is free
      if (board.authorize(uid))
      {
        Serial.println("Login successfull");
      } else {
        Serial.println("Login failed");
      }
      delay(1000);
    }
    else
    {
      // machine is busy
      FabMember member(uid);

      if (board.getMachine().getActiveUser().getUid() == member.getUid())
      {
        // we can logout. we should require that the card stays in the field for some seconds, to prevent accidental logout. maybe sound a buzzer?
        board.logout();
      }
      else
      {
        // user is not the same, display who is using it
        board.changeStatus(BoardStatus::ALREADY_IN_USE);
        delay(1000);
      }
      delay(1000);
    }
  }
  else
  {
    Serial.println(board.getRfid().IsNewCardPresent() ? "New card" : "No card");
    ready_for_a_new_card = true; // we should get SOME "no card" before flipping this flag
    // print status on lcd screen

    if (!board.getMachine().isFree())
    {
      board.changeStatus(BoardStatus::IN_USE);
    }
    else
    {
      board.changeStatus(BoardStatus::FREE);
    }
  }
  // select the card
}
