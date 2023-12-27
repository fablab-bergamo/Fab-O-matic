#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "FabServer.hpp"
#include "MachineConfig.hpp"
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

namespace fablabbg::Board
{
  // Global variables
#if (WOKWI_SIMULATION)
  MockRFIDWrapper rfid;
  FabServer server("Wokwi-GUEST", "", "127.0.0.1", 6);
  MockMQTTBroker broker;
#else
  RFIDWrapper rfid;
  FabServer server(secrets::wifi::ssid, secrets::wifi::password, secrets::mqtt::server);
#endif

  LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd(pins.lcd);

  MachineConfig config1(secrets::machine::machine_id,
                        secrets::machine::machine_type,
                        secrets::machine::machine_name,
                        pins.relay.ch1_pin, false,
                        secrets::machine::machine_topic,
                        conf::machine::DEFAULT_AUTO_LOGOFF_DELAY);

  Machine machine(config1, server);
  AuthProvider auth(secrets::cards::whitelist);
  BoardLogic logic;
  Tasks::Scheduler scheduler;
} // namespace Board

#endif // GLOBALS_H_
