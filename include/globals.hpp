#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "FabServer.hpp"
#include "Machine.hpp"
#include "LCDWrapper.hpp"
#include "conf.hpp"
#include "MockRFIDWrapper.hpp"
#include "RFIDWrapper.hpp"
#include "pins.hpp"
#include "secrets.hpp"
#include "AuthProvider.hpp"
#include "BoardLogic.hpp"
#include "Tasks.hpp"
#include "MockMQTTBroker.hpp"

// Global variables
namespace Board
{

#if (WOKWI_SIMULATION)
  MockRFIDWrapper rfid;
  FabServer server("Wokwi-GUEST", "", "127.0.0.1", 6);
  MockMQTTBroker broker;
#else
  RFIDWrapper rfid;
  FabServer server(secrets::wifi::ssid, secrets::wifi::password, secrets::mqtt::server);
#endif

  LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd(pins.lcd);

  const Machine::Config config1(secrets::machine::machine_id,
                                secrets::machine::machine_type,
                                secrets::machine::machine_name,
                                pins.relay.ch1_pin, false);

  Machine machine(config1, server);
  AuthProvider auth(secrets::cards::whitelist);
  BoardLogic logic;
  fablab::tasks::Scheduler scheduler;
} // namespace Board

#endif // GLOBALS_H_
