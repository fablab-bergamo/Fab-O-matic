#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "FabServer.hpp"
#include "MachineConfig.hpp"
#include "Machine.hpp"
#include "LCDWrapper.hpp"
#include "conf.hpp"
#include "RFIDWrapper.hpp"
#include "pins.hpp"
#include "secrets.hpp"
#include "AuthProvider.hpp"
#include "BoardLogic.hpp"
#include "Tasks.hpp"
#include "mock/MockMrfc522.hpp"
#include "SavedConfig.hpp"
#include "LCDWrapper.hpp"
#include "pins.hpp"
#include "LiquidCrystal.h"
#include "Mrfc522Driver.hpp"
#include "mock/MockMQTTBroker.hpp"

using namespace fablabbg;

namespace fablabbg::Board
{
  // Global variables
#if (WOKWI_SIMULATION)
  RFIDWrapper<MockMrfc522> rfid;
  MockMQTTBroker broker;
#else
  RFIDWrapper<Mrfc522Driver> rfid{};
#endif
  LCDWrapper<LiquidCrystal, conf::lcd::COLS, conf::lcd::ROWS> lcd{pins.lcd};
  FabServer server;
  BoardLogic logic;
  Tasks::Scheduler scheduler;
} // namespace Board

#endif // GLOBALS_H_
