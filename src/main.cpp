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

namespace fablabbg
{
  using namespace Board;
  using namespace Tasks;
  using namespace std::chrono;
  using Status = BoardLogic::Status;

  /// @brief Opens WiFi and server connection and updates board state accordingly
  void taskConnect()
  {
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
    // check if there is a card
    if (rfid.isNewCardPresent())
    {
      logic.onNewCard();
      return;
    }

    // No new card present
    logic.ready_for_a_new_card = true;
    if (machine.isFree())
    {
      logic.changeStatus(Status::FREE);
    }
    else
    {
      logic.changeStatus(Status::IN_USE);
    }
  }

  /// @brief blink led
  void taskBlink()
  {
    if (Board::server.isOnline())
      if (!Board::machine.allowed || Board::machine.maintenanceNeeded)
        Board::logic.set_led_color(64, 0, 0); // Red
      else
        Board::logic.set_led_color(0, 64, 0); // Green
    else
      Board::logic.set_led_color(127, 83, 16); // Orange

    Board::logic.invert_led(); // Blink when in use
  }

  /// @brief periodic check if the machine must be powered off
  void taskPoweroffCheck()
  {
    if (machine.canPowerOff())
    {
      machine.power(false);
    }
  }

  /// @brief periodic check if the machine must be powered off
  void taskPoweroffWarning()
  {
    if (machine.isShutdownImminent())
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
    if (machine.isAutologoffExpired())
    {
      Serial.printf("Auto-logging out user %s\r\n", machine.getActiveUser().holder_name.c_str());
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
      while (!rfid.init())
        delay(duration_cast<milliseconds>(conf::tasks::RFID_CHECK_PERIOD).count());
    }
  }

  /// @brief sends the MQTT alive message
  void taskMQTTAlive()
  {
    if (server.isOnline())
    {
      server.loop();
    }
  }

  void taskLcdRefresh()
  {
    BoardInfo bi = {Board::server.isOnline(), Board::machine.getPowerState(), Board::machine.isShutdownImminent()};
    Board::lcd.update(bi, true);
  }

#if (WOKWI_SIMULATION)

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
      delay(25);
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
  // Wokwi requires LCD refresh unlike real hardware
  Task t10("LCDRefresh", 2s, &taskLcdRefresh, scheduler, false);

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

        // Reconfigure FabServer which may have been initialized with default values
        server.configure(config);
      }
      else
      {
        Serial.println("Failed to save config to EEPROM");
      }
    }
  }

} // namespace fablabbg

using namespace fablabbg;

void setup()
{
  Serial.begin(conf::debug::SERIAL_SPEED_BDS); // Initialize serial communications with the PC for debugging.

  if (conf::debug::ENABLE_LOGS)
  {
    Serial.println("Starting setup!");
  }

  // Initialize hardware (RFID, LCD)
  if (!logic.board_init())
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

  // Now load network, machine id, mqtt config into the machine and server objects.
  if (!logic.loadConfig())
  {
    logic.changeStatus(Status::ERROR);
    logic.set_led_color(255, 0, 0);
    logic.led(true);
    logic.beep_failed();
#ifndef DEBUG
    // Cannot continue without valid configurations
    while (true)
      ;
#endif
  }

#if (WOKWI_SIMULATION)
  // Start MQTT server thread in simulation
  attr_mqtt_broker.stacksize = 3 * 1024;
  attr_mqtt_broker.detachstate = PTHREAD_CREATE_DETACHED;
  if (pthread_create(&thread_mqtt_broker, &attr_mqtt_broker, threadMQTTServer, NULL))
  {
    Serial.println("Error creating MQTT server thread");
  }
  // Enables LCD refresh for Wokwi
  t10.start();
#endif

  logic.beep_ok();
  server.connectWiFi();

  // Since the WiFiManager may have taken minutes, recompute the tasks schedule
  scheduler.restart();
}

void loop()
{
  scheduler.execute();
}
