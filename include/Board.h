#ifndef _BOARD_H_
#define _BOARD_H_

#include "FabUser.h"
#include "FabServer.h"
#include "Machine.h"
#include "LCDWrapper.h"
#include "conf.h"
#include "RFIDWrapper.h"
#include "pins.h"
#include "secrets.h"
#include "SPI.h"
#include "AuthProvider.h"
#include "MFRC522Driver.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522v2.h"

// Variables
namespace Board
{
    MFRC522DriverPinSimple rfid_simple_driver(pins.mfrc522.sda_pin);
    MFRC522DriverSPI spi_rfid_driver{rfid_simple_driver}; // Create SPI driver.
    MFRC522 mfrc522{spi_rfid_driver};         // Create MFRC522 instance.
    RFIDWrapper rfid;
    LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS>::Config config_lcd(pins.lcd.rs_pin, pins.lcd.en_pin, pins.lcd.d0_pin, pins.lcd.d1_pin, pins.lcd.d2_pin, pins.lcd.d3_pin);
    LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd(config_lcd);
    FabServer server(secrets::wifi::ssid, secrets::wifi::password);
    Machine::Config config1{secrets::machine::machine_id, Machine::MachineType::PRINTER3D, secrets::machine::machine_name, pins.relay.ch1_pin, false};
    Machine machine(config1);
    AuthProvider auth(secrets::cards::whitelist);
}
#endif