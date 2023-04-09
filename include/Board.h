#ifndef _BOARD_H_
#define _BOARD_H_

#include "FabMember.h"
#include "FabServer.h"
#include "Machine.h"
#include "LCDWrapper.h"
#include "conf.h"
#include "RFIDWrapper.h"
#include "pins.h"
#include "secrets.h"

namespace Board
{
    // Variables
    MFRC522DriverPinSimple rfid_simple_driver(pins.mfrc522.cs_pin);
    MFRC522DriverSPI spi_rfid_driver{rfid_simple_driver}; // Create SPI driver.
    MFRC522 mfrc522{spi_rfid_driver};         // Create MFRC522 instance.
    RFIDWrapper rfid;
    LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS>::Config config_lcd(pins.lcd.rs_pin, pins.lcd.en_pin, pins.lcd.d0_pin, pins.lcd.d1_pin, pins.lcd.d2_pin, pins.lcd.d3_pin);
    LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd(config_lcd);
    FabServer server(secrets::machine_data::whitelist, secrets::wifi::ssid, secrets::wifi::password);
    Machine::Config config1{secrets::machine_data::machine_id, Machine::MachineType::PRINTER3D, pins.relay.ch1_pin, false};
    Machine machine(config1);
}
#endif