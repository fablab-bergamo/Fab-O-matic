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
#if (WOKWI_SIMULATION)
#include "mock/MockMQTTBroker.hpp"
#endif

using namespace fablabbg;

namespace fablabbg::Board
{
  // Global variables
#if (WOKWI_SIMULATION)
  DRAM_ATTR RFIDWrapper<MockMrfc522> rfid;
  DRAM_ATTR MockMQTTBroker broker;
#else
  DRAM_ATTR RFIDWrapper<Mrfc522Driver> rfid{};
#endif
  DRAM_ATTR LCDWrapper<LiquidCrystal> lcd{pins.lcd};
  DRAM_ATTR BoardLogic logic;
  DRAM_ATTR Tasks::Scheduler scheduler;
} // namespace fablabbg::Board

#endif // GLOBALS_H_
