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
#if (MQTT_SIMULATION)
#include "mock/MockMQTTBroker.hpp"
#endif

namespace fabomatic::Board
{
  // Global variables
#if (MQTT_SIMULATION)
  MockMQTTBroker broker;
#endif

#if (RFID_SIMULATION)
  RFIDWrapper<MockMrfc522> rfid;
#else
  RFIDWrapper<Mrfc522Driver> rfid{};
#endif

  LCDWrapper<LiquidCrystal> lcd{pins.lcd};
  BoardLogic logic{};
  Tasks::Scheduler scheduler{};
} // namespace fabomatic::Board

#endif // GLOBALS_HPP_
