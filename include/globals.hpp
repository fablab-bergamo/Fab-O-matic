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
#include "SavedConfig.hpp"

namespace fablabbg::Board
{
  // Global variables
#if (WOKWI_SIMULATION)
  MockRFIDWrapper rfid;
  FabServer server;
  MockMQTTBroker broker;
#else
  RFIDWrapper rfid;
  FabServer server;
#endif

  LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd(pins.lcd);
  Machine machine;
  AuthProvider auth(secrets::cards::whitelist);
  BoardLogic logic;
  Tasks::Scheduler scheduler;
} // namespace Board

#endif // GLOBALS_H_
