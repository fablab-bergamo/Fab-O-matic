#include <cstdint>
#include <string>
#include <array>
#include "pthread.h"

#include <esp_task_wdt.h>
#include <WiFiManager.h>

#include "globals.hpp"
#include "pins.hpp"
#include "BoardLogic.hpp"
#include "Tasks.hpp"
#include "SavedConfig.hpp"
#include "Mrfc522Driver.hpp"
#include "mock/MockMQTTBroker.hpp"

namespace fablabbg
{
  using namespace Board;
  using namespace Tasks;
  using namespace std::chrono;
  using Status = BoardLogic::Status;

  namespace Board
  {
    // Only main.cpp instanciates the variables through Board.h file
#if (WOKWI_SIMULATION)
    extern RFIDWrapper<MockMrfc522> rfid;
#else
    extern RFIDWrapper<Mrfc522Driver> rfid;
#endif
    extern Scheduler scheduler;
    extern BoardLogic logic;
  }

  /// @brief Opens WiFi and server connection and updates board state accordingly
  void taskConnect()
  {
    auto &server = logic.getServer();
    if (!server.isOnline())
    {
      // connection to wifi
      logic.changeStatus(Status::CONNECTING);

      // Try to connect
      server.connect();
      // Refresh after connection
      logic.changeStatus(server.isOnline() ? Status::CONNECTED : Status::OFFLINE);

      // Briefly show to the user
      delay(500);
    }

    if (server.isOnline())
    {
      Serial.println("taskConnect - online, calling refreshFromServer");
      // Get machine data from the server if it is online
      logic.refreshFromServer();
    }
  }

  /// @brief periodic check for new RFID card
  void taskCheckRfid()
  {
    Board::logic.checkRfid();
  }

  /// @brief blink led
  void taskBlink()
  {
    Board::logic.blinkLed();
  }

  /// @brief periodic check if the machine must be powered off
  void taskPoweroffCheck()
  {
    Board::logic.checkPowerOff();
  }

  /// @brief periodic check if the machine must be powered off
  void taskPoweroffWarning()
  {
    if (Board::logic.getMachine().isShutdownImminent())
    {
      logic.beep_failed();
      if (conf::debug::ENABLE_LOGS)
        Serial.println("Machine is about to shutdown");
    }
  }

  /// @brief periodic check if the user shall be logged off
  void taskLogoffCheck()
  {
    // auto logout after delay
    auto &machine = Board::logic.getMachine();
    if (machine.isAutologoffExpired())
    {
      Serial.printf("Auto-logging out user %s\r\n", machine.getActiveUser().holder_name.data());
      logic.logout();
      logic.beep_failed();
    }
  }

  /// @brief Keep the ESP32 HW watchdog alive.
  /// If code gets stuck this will cause an automatic reset.
  void taskEspWatchdog()
  {
    static auto initialized = false;

    if (conf::tasks::WATCHDOG_TIMEOUT > 0s)
    {
      if (!initialized)
      {
        esp_task_wdt_init(duration_cast<seconds>(conf::tasks::WATCHDOG_TIMEOUT).count(), true); // enable panic so ESP32 restarts
        esp_task_wdt_add(NULL);                                                                 // add current thread to WDT watch
        initialized = true;
      }
      esp_task_wdt_reset(); // Signal the hardware watchdog
    }
  }

  /// @brief checks the RFID chip status and re-init it if necessary.
  void taskRfidWatchdog()
  {
    if (!rfid.selfTest())
    {
      Serial.println("RFID chip failure");

      // Infinite retry until success or hw watchdog timeout
      while (!rfid.init_rfid())
        delay(duration_cast<milliseconds>(conf::tasks::RFID_CHECK_PERIOD).count());
    }
  }

  /// @brief sends the MQTT alive message
  void taskMQTTAlive()
  {
    auto &server = logic.getServer();
    if (server.isOnline())
    {
      server.loop();
    }
  }

  void taskLcdRefresh()
  {
    Board::logic.refreshLCD();
  }

#if (WOKWI_SIMULATION)
  void taskRFIDCardSim()
  {
    static uid_t logged_uid = card::INVALID;
    auto &driver = rfid.getDriver();

    if (logged_uid == card::INVALID)
    {
      // Select random card every X times
      if (random(0, 100) < 5)
      {
        auto [card_uid, level, name] = secrets::cards::whitelist[random(0, secrets::cards::whitelist.size())];
        logged_uid = card_uid;
        driver.setUid(card_uid, milliseconds(500));
      }
    }
    else
    {
      // Select logged-in card every X times
      if (random(0, 100) < 5)
      {
        driver.setUid(logged_uid, milliseconds(500));
        logged_uid = card::INVALID;
      }
    }
  }

  pthread_t thread_mqtt_broker;
  pthread_attr_t attr_mqtt_broker;

  void *threadMQTTServer(void *arg)
  {
    if (conf::debug::ENABLE_LOGS)
      Serial.println("threadMQTTServer started");

    delay(2000);
    while (true)
    {
      // Check if the server is online
      if (!Board::broker.isRunning())
      {
        Board::broker.start();
      }
      else
      {
        Board::broker.update();
      }
      delay(50);
    }
  }

  void startMQTTBrocker()
  {
    // Start MQTT server thread in simulation
    attr_mqtt_broker.stacksize = 3 * 1024; // Required for ESP32-S2
    attr_mqtt_broker.detachstate = PTHREAD_CREATE_DETACHED;
    if (pthread_create(&thread_mqtt_broker, &attr_mqtt_broker, threadMQTTServer, NULL))
    {
      Serial.println("Error creating MQTT server thread");
    }
  }
#endif

  // Tasks definitions
  //
  // They will be executed at the required frequency during loop()->scheduler.execute() call
  // The scheduler will take care of the timing and will call the task callback

  Task t1("RFIDChip", conf::tasks::RFID_CHECK_PERIOD, &taskCheckRfid, scheduler, true);
  Task t2("Wifi/MQQT init", conf::tasks::MQTT_REFRESH_PERIOD, &taskConnect, scheduler, true, 20s);
  Task t3("Poweroff", 1s, &taskPoweroffCheck, scheduler, true);
  Task t4("Logoff", 1s, &taskLogoffCheck, scheduler, true);
  // Hardware watchdog will run at one third the frequency
  Task t5("Watchdog", conf::tasks::WATCHDOG_TIMEOUT / 3, &taskEspWatchdog, scheduler, true);
  Task t6("Selftest", conf::tasks::RFID_SELFTEST_PERIOD, &taskRfidWatchdog, scheduler, true);
  Task t7("PoweroffWarning", conf::machine::DELAY_BETWEEN_BEEPS, &taskPoweroffWarning, scheduler, true);
  Task t8("MQTT keepalive", 1s, &taskMQTTAlive, scheduler, true);
  Task t9("LED", 1s, &taskBlink, scheduler, true);

#if (WOKWI_SIMULATION)
  // Wokwi requires LCD refresh unlike real hardware
  Task t10("LCDRefresh", 2s, &taskLcdRefresh, scheduler, true);
  Task t11("RFIDCardsSim", 1s, &taskRFIDCardSim, scheduler, true, 30s);
#endif

  // flag for saving data
  bool shouldSaveConfig = false;

  // callback notifying us of the need to save config
  void saveConfigCallback()
  {
    shouldSaveConfig = true;
  }

  // Called just before the webportal is started after failed WiFi connection
  void configModeCallback(WiFiManager *myWiFiManager)
  {
    Serial.println("Entering config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
    logic.changeStatus(Status::PORTAL_STARTING);
  }

  // Starts the WiFi portal for configuration if needed
  void config_portal()
  {
    WiFiManager wifiManager;
    SavedConfig config = SavedConfig::LoadFromEEPROM().value_or(SavedConfig::DefaultConfig());

    WiFiManagerParameter custom_mqtt_server("Broker", "MQTT Broker address", config.mqtt_server, sizeof(config.mqtt_server));
    WiFiManagerParameter custom_mqtt_topic("Topic", "MQTT Switch topic (leave empty to disable)", config.machine_topic, sizeof(config.machine_topic));
    WiFiManagerParameter custom_machine_id("MachineID", "Machine ID", config.machine_id, sizeof(config.machine_id));

    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_topic);
    wifiManager.addParameter(&custom_machine_id);

    wifiManager.setTimeout(duration_cast<seconds>(conf::tasks::PORTAL_CONFIG_TIMEOUT).count());
    wifiManager.setConnectRetries(3);  // 3 retries
    wifiManager.setConnectTimeout(10); // 10 seconds
    wifiManager.setCountry("IT");
    wifiManager.setTitle("FabLab Bergamo - RFID arduino");
    wifiManager.setCaptivePortalEnable(true);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    if (conf::debug::FORCE_PORTAL_RESET)
    {
      wifiManager.resetSettings();
    }

#if (WOKWI_SIMULATION)
    wifiManager.setDebugOutput(true);
    wifiManager.resetSettings();
    wifiManager.setTimeout(10); // fail fast for debugging
#endif

    if (wifiManager.autoConnect())
    {
      logic.changeStatus(Status::PORTAL_OK);
      delay(1000);
    }
    else
    {
      logic.changeStatus(Status::PORTAL_FAILED);
      delay(3000);
    }

    if (shouldSaveConfig)
    {
      // save SSID data from WiFiManager
      strncpy(config.ssid, WiFi.SSID().c_str(), sizeof(config.ssid));
      strncpy(config.password, WiFi.psk().c_str(), sizeof(config.password));

      // read updated parameters
      strncpy(config.mqtt_server, custom_mqtt_server.getValue(), sizeof(config.mqtt_server));
      strncpy(config.machine_topic, custom_mqtt_topic.getValue(), sizeof(config.machine_topic));
      strncpy(config.machine_id, custom_machine_id.getValue(), sizeof(config.machine_id));

      // save the custom parameters to EEPROM
      if (config.SaveToEEPROM())
      {
        Serial.println("Config saved to EEPROM");

        // Reconfigure all settings after changes
        logic.reconfigure();
      }
      else
      {
        Serial.println("Failed to save config to EEPROM");
      }
    }
  }
} // namespace fablabbg

using namespace fablabbg;

#ifndef UNIT_TEST
void setup()
{
  Serial.begin(conf::debug::SERIAL_SPEED_BDS); // Initialize serial communications with the PC for debugging.

  if (conf::debug::ENABLE_LOGS)
  {
    Serial.println("Starting setup!");
  }

  // Initialize hardware (RFID, LCD)
  auto success = logic.configure(Board::rfid, Board::lcd);
  success &= logic.board_init();

  if (!success)
  {
    logic.changeStatus(Status::ERROR_HW);
    logic.set_led_color(255, 0, 0);
    logic.led(true);
    logic.beep_failed();
#ifndef DEBUG
    // Cannot continue without RFID or LCD
    while (true)
      ;
#endif
  }

  // Network configuration setup
  config_portal();

#if (WOKWI_SIMULATION)
  startMQTTBrocker();
#endif

  logic.beep_ok();

  // Join the AP and try to connect to broker
  logic.getServer().connect();

  // Since the WiFiManager may have taken minutes, recompute the tasks schedule
  scheduler.restart();
}

void loop()
{
  scheduler.execute();
}
#endif // UNIT_TEST