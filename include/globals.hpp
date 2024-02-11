#ifndef GLOBALS_HPP_
#define GLOBALS_HPP_

#include "AuthProvider.hpp"
#include "BoardLogic.hpp"
#include "FabBackend.hpp"
#include "LCDWrapper.hpp"
#include "LiquidCrystal.h"
#include "Machine.hpp"
#include "MachineConfig.hpp"
#include "Mrfc522Driver.hpp"
#include "RFIDWrapper.hpp"
#include "SavedConfig.hpp"
#include "Tasks.hpp"
#include "conf.hpp"
#include "mock/MockMrfc522.hpp"
#include "pins.hpp"
#include "secrets.hpp"
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

#endif // GLOBALS_HPP_
