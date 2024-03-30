#include "pthread.h"
#include <array>
#include <cstdint>
#include <string>

#include "ArduinoOTA.h"
#include <WiFiManager.h>
#include <esp_task_wdt.h>

#include "BoardLogic.hpp"
#include "Logging.hpp"
#include "Mrfc522Driver.hpp"
#include "SavedConfig.hpp"
#include "Tasks.hpp"
#include "globals.hpp"
#include "mock/MockMQTTBroker.hpp"
#include "pins.hpp"

// For ArduinoOTA
const char *ssid = fablabbg::secrets::credentials::ssid.data();
const char *password = fablabbg::secrets::credentials::password.data();

using namespace std::chrono_literals;

namespace fablabbg
{
  using Scheduler = Tasks::Scheduler;
  using Task = Tasks::Task;
  using Status = BoardLogic::Status;

  namespace Board
  {
    // Only main.cpp instanciates the variables through Board.h file
#if (RFID_SIMULATION)
    extern RFIDWrapper<MockMrfc522> rfid;
#else
    extern RFIDWrapper<Mrfc522Driver> rfid;
#endif
    extern Scheduler scheduler;
    extern BoardLogic logic;
  } // namespace Board

  /// @brief Opens WiFi and server connection and updates board state accordingly
  void taskConnect()
  {
    auto &server = Board::logic.getServer();
    if (!server.isOnline())
    {
      // connection to wifi
      Board::logic.changeStatus(Status::Connecting);

      // Try to connect
      server.connect();
      // Refresh after connection
      Board::logic.changeStatus(server.isOnline() ? Status::Connected : Status::Offline);

      // Briefly show to the user
      Tasks::task_delay(conf::lcd::SHORT_MESSAGE_DELAY);
    }

    if (server.isOnline())
    {
      ESP_LOGI(TAG, "taskConnect - online, calling refreshFromServer");
      // Get machine data from the server if it is online
      Board::logic.refreshFromServer();
      if (auto &machine = Board::logic.getMachine(); !machine.isFree())
      {
        auto response = Board::logic.getServer().inUse(
            machine.getActiveUser().card_uid,
            machine.getUsageDuration());
        if (!response)
        {
          ESP_LOGE(TAG, "taskConnect - inUse failed");
        }
      }
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
      Board::logic.changeStatus(Status::ShuttingDown);
      Board::logic.beep_failed();
      if (conf::debug::ENABLE_LOGS)
        ESP_LOGI(TAG, "Machine is about to shutdown");
    }

    if (Board::logic.getRebootRequest())
    {
      if (Board::logic.getMachine().getPowerState() == Machine::PowerState::PoweredOff)
      {
        ESP_LOGI(TAG, "Rebooting as per request");
        ESP.restart();
      }
    }
  }

  /// @brief periodic check if the user shall be logged off
  void taskLogoffCheck()
  {
    // auto logout after delay
    auto &machine = Board::logic.getMachine();
    if (machine.isAutologoffExpired())
    {
      ESP_LOGI(TAG, "Auto-logging out user %s\r\n", machine.getActiveUser().holder_name.data());
      Board::logic.logout();
      Board::logic.beep_failed();
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
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(conf::tasks::WATCHDOG_TIMEOUT).count();
        esp_task_wdt_init(secs, true); // enable panic so ESP32 restarts
        ESP_LOGI(TAG, "taskEspWatchdog - initialized %lld seconds", secs);
        esp_task_wdt_add(NULL); // add current thread to WDT watch
        initialized = true;
      }
      esp_task_wdt_reset(); // Signal the hardware watchdog
    }
  }

  /// @brief checks the RFID chip status and re-init it if necessary.
  void taskRfidWatchdog()
  {
    if (!Board::rfid.selfTest())
    {
      ESP_LOGE(TAG, "RFID chip failure");

      // Infinite retry until success or hw watchdog timeout
      while (!Board::rfid.init_rfid())
      {
        Tasks::task_delay(conf::tasks::RFID_CHECK_PERIOD);
#ifdef DEBUG
        break;
#endif
      }
    }
  }

  /// @brief sends the MQTT alive message
  void taskMQTTAlive()
  {
    auto &server = Board::logic.getServer();
    if (server.isOnline())
    {
      server.loop();
    }
  }

  void taskIsAlive()
  {
    // notify the server that the machine is still alive
    auto &server = Board::logic.getServer();
    if (server.isOnline())
    {
      if (auto resp = server.alive(); !resp)
      {
        ESP_LOGE(TAG, "taskIsAlive - alive failed");
      }
    }
  }

  void taskFactoryReset()
  {
    static auto start = std::chrono::system_clock::now();

    if constexpr (pins.buttons.factory_defaults_pin == NO_PIN)
      return;

    pinMode(pins.buttons.factory_defaults_pin, INPUT_PULLUP);

    if (digitalRead(pins.buttons.factory_defaults_pin) == LOW)
    {
      if (std::chrono::system_clock::now() - start > conf::tasks::FACTORY_DEFAULTS_DELAY)
      {
        ESP_LOGW(TAG, "Factory reset requested");
        if (auto config = SavedConfig::DefaultConfig(); config.SaveToEEPROM())
        {
          Board::logic.setRebootRequest(true);
          start = std::chrono::system_clock::now();

          Board::logic.changeStatus(Status::FactoryDefaults);
          Tasks::task_delay(1s);
        }
        else
        {
          ESP_LOGE(TAG, "Factory reset failed");
          Board::logic.changeStatus(Status::Error);
          Tasks::task_delay(1s);
        }
        return;
      }
      Board::logic.blinkLed(255, 165, 0); // Blink orange
      ESP_LOGI(TAG, "Factory reset pending...");
    }
    else
    {
      start = std::chrono::system_clock::now();
    }
  }

#if (RFID_SIMULATION)
  void taskRFIDCardSim()
  {
    static uid_t logged_uid = card::INVALID;
    auto &driver = Board::rfid.getDriver();

    if (logged_uid == card::INVALID)
    {
      // Select random card every X times
      if (random(0, 100) < 5)
      {
        auto [card_uid, level, name] = secrets::cards::whitelist[random(0, secrets::cards::whitelist.size())];
        logged_uid = card_uid;
        driver.setUid(card_uid, 500ms);
      }
    }
    else
    {
      // Select logged-in card every X times
      if (random(0, 100) < 5)
      {
        driver.setUid(logged_uid, 500ms);
        logged_uid = card::INVALID;
      }
    }
  }
#endif

#if (MQTT_SIMULATION)
  void *threadMQTTServer(void *arg)
  {
    ESP_LOGI(TAG, "threadMQTTServer started");

    delay(2000);
    while (true)
    {
      Board::broker.mainLoop();
      delay(50);
    }
  }

  void startMQTTBrocker()
  {
    static pthread_t thread_mqtt_broker;
    static pthread_attr_t attr_mqtt_broker;
    // Start MQTT server thread in simulation
    attr_mqtt_broker.stacksize = 3 * 1024; // Required for ESP32-S2
    attr_mqtt_broker.detachstate = PTHREAD_CREATE_DETACHED;
    if (pthread_create(&thread_mqtt_broker, &attr_mqtt_broker, threadMQTTServer, NULL))
    {
      ESP_LOGE(TAG, "Error creating MQTT server thread");
    }
  }
#endif

  // Tasks definitions
  //
  // They will be executed at the required frequency during loop()->scheduler.execute() call
  // The scheduler will take care of the timing and will call the task callback

  const Task t_rfid("RFIDChip", conf::tasks::RFID_CHECK_PERIOD, &taskCheckRfid, Board::scheduler, true);
  const Task t_net("Wifi/MQTT", conf::tasks::MQTT_REFRESH_PERIOD, &taskConnect, Board::scheduler, true, 20s);
  const Task t_powoff("Poweroff", 1s, &taskPoweroffCheck, Board::scheduler, true);
  const Task t_log("Logoff", 1s, &taskLogoffCheck, Board::scheduler, true);
  // Hardware watchdog will run at one third the frequency
  Task t_wdg("Watchdog", conf::tasks::WATCHDOG_TIMEOUT / 3, &taskEspWatchdog, Board::scheduler, false);
  const Task t_test("Selftest", conf::tasks::RFID_SELFTEST_PERIOD, &taskRfidWatchdog, Board::scheduler, true);
  const Task t_warn("PoweroffWarning", conf::machine::DELAY_BETWEEN_BEEPS, &taskPoweroffWarning, Board::scheduler, true);
  const Task t_mqtt("MQTT keepalive", 1s, &taskMQTTAlive, Board::scheduler, true);
  const Task t_led("LED", 1s, &taskBlink, Board::scheduler, true);
  const Task t_rst("FactoryReset", 500ms, &taskFactoryReset, Board::scheduler, pins.buttons.factory_defaults_pin != NO_PIN);
  const Task t_alive("IsAlive", conf::tasks::MQTT_ALIVE_PERIOD, &taskIsAlive, Board::scheduler, true, 30s);
#if (RFID_SIMULATION)
  const Task t_sim("RFIDCardsSim", 1s, &taskRFIDCardSim, Board::scheduler, true, 30s);
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
    ESP_LOGI(TAG, "Entering portal config mode");
    ESP_LOGD(TAG, "%s", WiFi.softAPIP().toString().c_str());
    ESP_LOGD(TAG, "%s", myWiFiManager->getConfigPortalSSID().c_str());
    Board::logic.changeStatus(Status::PortalStarting);
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

    wifiManager.setTimeout(std::chrono::duration_cast<std::chrono::seconds>(conf::tasks::PORTAL_CONFIG_TIMEOUT).count());
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

#if (PINS_WOKWI)
    wifiManager.setDebugOutput(true);
    wifiManager.resetSettings();
    wifiManager.setTimeout(10); // fail fast for debugging
#endif

    if (wifiManager.autoConnect())
    {
      Board::logic.changeStatus(Status::PortalSuccess);
      delay(1000);
    }
    else
    {
      Board::logic.changeStatus(Status::PortalFailed);
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
        ESP_LOGD(TAG, "Config saved to EEPROM");

        // Reconfigure all settings after changes
        Board::logic.reconfigure();
      }
      else
      {
        ESP_LOGE(TAG, "Failed to save config to EEPROM");
      }
    }
  }

  void OTAComplete()
  {
    ESP_LOGI(TAG, "OTA complete, reboot requested");
    Board::logic.setRebootRequest(true);
  }

  void setupOTA()
  {
    std::stringstream ss{};
    // Hostname is BOARD + machine_id (which shall be unique) e.g. BOARD1
    ss << conf::default_config::hostname.data() << conf::default_config::machine_id.id;
    ArduinoOTA.setHostname(ss.str().c_str());
    ArduinoOTA.onStart([]()
                       { Board::logic.changeStatus(Status::OTAStarting); });
    ArduinoOTA.onEnd(OTAComplete);
    ArduinoOTA.onError([](ota_error_t error)
                       { Board::logic.changeStatus(Status::OTAError); });
    ArduinoOTA.setMdnsEnabled(true);
    ArduinoOTA.setRebootOnSuccess(false);
    ArduinoOTA.setTimeout(45000);
    ArduinoOTA.begin();
  }
} // namespace fablabbg

#ifndef PIO_UNIT_TESTING
void setup()
{
  using Status = fablabbg::BoardLogic::Status;
  auto &logic = fablabbg::Board::logic;
  auto &scheduler = fablabbg::Board::scheduler;
  auto &rfid = fablabbg::Board::rfid;
  auto &lcd = fablabbg::Board::lcd;

  Serial.begin(fablabbg::conf::debug::SERIAL_SPEED_BDS); // Initialize serial communications with the PC for debugging.
  delay(5000);

  if constexpr (fablabbg::conf::debug::ENABLE_LOGS)
  {
    Serial.setDebugOutput(true);
    ESP_LOGD(TAG, "Starting setup!");
  }

  if constexpr (fablabbg::conf::debug::LOAD_EEPROM_DEFAULTS)
  {
    auto defaults = fablabbg::SavedConfig::DefaultConfig();
    ESP_LOGW(TAG, "Forcing EEPROM defaults : %s", defaults.toString().c_str());
    defaults.SaveToEEPROM();
  }

  logic.blinkLed();

  // Initialize hardware (RFID, LCD)
  auto success = logic.configure(rfid, lcd);
  success &= logic.board_init();

  logic.changeStatus(Status::Booting);

  if (!success)
  {
    logic.changeStatus(Status::ErrorHardware);
    logic.beep_failed();
    logic.blinkLed();
#ifndef DEBUG
    // Cannot continue without RFID or LCD
    while (true)
    {
      fablabbg::Tasks::task_delay(500ms); // Allow OTA
      Serial.println("Error initializing hardware");
    }
#endif
  }

  // Network configuration setup
  fablabbg::config_portal();

#if (MQTT_SIMULATION)
  fablabbg::startMQTTBrocker();
#endif
  logic.beep_ok();

  // Join the AP and try to connect to broker
  logic.getServer().connect();

  fablabbg::t_wdg.start(); // Enable the HW watchdog

  fablabbg::setupOTA();

  // Since the WiFiManager may have taken minutes, recompute the tasks schedule
  scheduler.restart();
}

void loop()
{
  fablabbg::Board::scheduler.execute();
  ArduinoOTA.handle();
}
#endif // PIO_UNIT_TESTING