#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "FabServer.hpp"
#include "MachineConfig.hpp"
#include "Machine.hpp"
#include "LCDWrapper.hpp"
#include "conf.hpp"
#include "mock/MockRFIDWrapper.hpp"
#include "RFIDWrapper.hpp"
#include "pins.hpp"
#include "secrets.hpp"
#include "AuthProvider.hpp"
#include "BoardLogic.hpp"
#include "Tasks.hpp"
#include "mock/MockMQTTBroker.hpp"
#include "SavedConfig.hpp"
#include "LCDWrapper.hpp"
#include "pins.hpp"

using namespace fablabbg;

namespace fablabbg::Board
{
  // Global variables
#if (WOKWI_SIMULATION)
  MockRFIDWrapper rfid;
  MockMQTTBroker broker;
#else
  RFIDWrapper rfid;
#endif
  LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd{pins.lcd};
  FabServer server;
  BoardLogic logic;
  Tasks::Scheduler scheduler;
} // namespace Board

#endif // GLOBALS_H_
