#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"

#include "pins.h"
#include "secrets.h"
#include "Machine.h"
#include "FabServer.h"
#include "LCDWrapper.h"

#include <cstdint>
#include <string>
#include <array>
#include <chrono>

#include <LiquidCrystal.h>
#include <WiFi.h>
#include "conf.h"

MFRC522DriverPinSimple ss_pin(pins::mfrc522::ss_pin); // Configurable, see typical pin layout above.

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver.
// MFRC522DriverI2C driver{}; // Create I2C driver.
MFRC522 mfrc522{driver}; // Create MFRC522 instance.

LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> LCD(pins::lcd::rs_pin, pins::lcd::en_pin, pins::lcd::d0_pin, pins::lcd::d1_pin, pins::lcd::d2_pin, pins::lcd::d3_pin);

FabServer server(secrets::machine_data::whitelist, secrets::wifi::ssid, secrets::wifi::password);

Machine machine(secrets::machine_data::machine_id, Machine::PRINTER3D, conf::machine::CONTROL_PIN_1, false);

FabMember candidate_user = FabMember();

static bool ready_for_a_new_card = true;



void setup()
{

  Serial.begin(115200); // Initialize serial communications with the PC for debugging.
  LCD.begin();

  mfrc522.PCD_Init(); // Init MFRC522 board.

  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  // connection to wifi
  LCD.state(LCDState::CONNECTING);
  LCD.update(server, candidate_user, machine);
  
  LCD.showConnection(true);
  LCD.setConnectionState(server.isOnline());

  LCD.state(server.isOnline() ? LCDState::CONNECTED : LCDState::OFFLINE);
  LCD.update(server, candidate_user, machine);
  delay(1000);
}

void loop()
{
  delay(100);

  // check if there is a card
  if (mfrc522.PICC_IsNewCardPresent())
  {
    // if there is a "new" card (could be the same that stayed in the field)
    if (!mfrc522.PICC_ReadCardSerial() || !ready_for_a_new_card)
    {
      return;
    }
    ready_for_a_new_card = false;

    // Acquire the UID of the card
    candidate_user.setUidFromArray(mfrc522.uid.uidByte);

    if (machine.isFree())
    {
      // machine is free
      if (server.isAuthorized(candidate_user))
      {
        machine.login(candidate_user);
        LCD.state(LCDState::LOGGED_IN);
        LCD.update(server, candidate_user, machine);
        delay(1000);
      }
      else
      {
        LCD.state(LCDState::LOGIN_DENIED);
        LCD.update(server, candidate_user, machine);
        delay(1000);
      }
    }
    else
    {
      // machine is busy
      if (machine.getActiveUser().getUid() == candidate_user.getUid())
      {
        // we can logout. we should require that the card stays in the field for some seconds, to prevent accidental logout. maybe sound a buzzer?
        machine.logout();
        LCD.state(LCDState::LCDStateType::LOGOUT);
        LCD.update(server, candidate_user, machine);
      }
      else
      {
        // user is not the same, display who is using it
        LCD.state(LCDState::ALREADY_IN_USE);
        LCD.update(server, candidate_user, machine);
        delay(1000);
      }
      delay(1000);
    }
  }
  else
  {
    Serial.println(mfrc522.PICC_IsNewCardPresent() ? "New card" : "No card");
    ready_for_a_new_card = true; // we should get SOME "no card" before flipping this flag
    // print status on lcd screen

    if (!machine.isFree())
    {
      LCD.state(LCDState::IN_USE);
      LCD.update(server, candidate_user, machine);
    }
    else
    {
      LCD.state(LCDState::FREE);
      LCD.update(server, candidate_user, machine);
    }
  }
  // select the card
}
