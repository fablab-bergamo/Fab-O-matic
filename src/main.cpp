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

std::string convertSecondsToHHMMSS(unsigned long milliseconds)
{
  //! since something something does not support to_string we have to resort to ye olde cstring stuff
  char buffer[9];
  unsigned long seconds = milliseconds / 1000;
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", (int)(seconds / 3600), (int)((seconds % 3600) / 60), (int)(seconds % 60));

  std::string result(buffer);
  return result;
}

void setup()
{

  Serial.begin(115200); // Initialize serial communications with the PC for debugging.
  LCD.begin();

  mfrc522.PCD_Init(); // Init MFRC522 board.

  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  // connection to wifi
  LCD.setRow(0, "Connettendo...");
  LCD.update(server, candidate_user);
  // server.connect();
  LCD.showConnection(true);
  LCD.setConnectionState(server.isOnline());
  LCD.setRow(1, server.isOnline() ? "CONNECTED" : "OFFLINE MODE");
  LCD.update(server, candidate_user);
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
        LCD.setRow(0, "Inizio uso");
        LCD.setRow(1, candidate_user.getName());
        LCD.update(server, candidate_user);
        delay(1000);
      }
      else
      {
        LCD.setRow(0, "Negato");
        LCD.setRow(1, "Carta sconosciuta");
        LCD.update(server, candidate_user);
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
        LCD.update(server, candidate_user);
      }
      else
      {
        // user is not the same
        LCD.setRow(0, "Negato");
        LCD.setRow(1, "In uso");
        LCD.update(server, candidate_user);
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
      char buffer[conf::lcd::COLS];
      snprintf(buffer, sizeof(buffer), "Ciao %s", machine.getActiveUser().getName().c_str());
      LCD.setRow(0, buffer);
      LCD.setRow(1, convertSecondsToHHMMSS(machine.getUsageTime()));
      LCD.update(server, candidate_user);
    }
    else
    {
      LCD.setRow(0, server.isOnline() ? "Disponibile" : "OFFLINE");
      LCD.setRow(1, "Avvicina carta");
      LCD.update(server, candidate_user);
    }
  }
  // select the card
}
