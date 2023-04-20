#ifndef BOARD_H_
#define BOARD_H_

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

// Global variables
namespace Board
{
    RFIDWrapper rfid;
    LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd(pins.lcd);

    FabServer server(secrets::wifi::ssid, secrets::wifi::password, secrets::wifi::server_ip);

    Machine::Config config1(secrets::machine::machine_id,
                            secrets::machine::machine_type,
                            secrets::machine::machine_name,
                            pins.relay.ch1_pin, false);

    Machine machine(config1);
    AuthProvider auth(secrets::cards::whitelist);
} // namespace Board
#endif // BOARD_H_
