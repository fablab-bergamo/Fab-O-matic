#include "pthread.h"
#include <array>
#include <cstdint>
#include <iostream>
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
const char *const ssid = fablabbg::secrets::credentials::ssid.data();
const char *const password = fablabbg::secrets::credentials::password.data();

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

  // Pre-declaration
  void openConfigPortal(bool force_reset, bool disable_portal);

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
      Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY);
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
      Board::logic.beepFail();
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
      Board::logic.beepFail();
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
      while (!Board::rfid.rfidInit())
      {
        Board::logic.changeStatus(Status::ErrorHardware);
        Tasks::delay(conf::tasks::RFID_CHECK_PERIOD);
#ifdef DEBUG
        break;
#endif
      }
    }
  }

  /// @brief sends the MQTT alive message
  void taskMQTTClientLoop()
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
    if (!Board::logic.saveRfidCache())
    {
      ESP_LOGE(TAG, "taskIsAlive - saveRfidCache failed");
    }
    if constexpr (conf::debug::ENABLE_LOGS)
    {
      ESP_LOGD(TAG, "taskIsAlive - free heap: %d", ESP.getFreeHeap());
    }
  }

  void taskFactoryReset()
  {
    if constexpr (pins.buttons.factory_defaults_pin == NO_PIN)
      return;

    pinMode(pins.buttons.factory_defaults_pin, INPUT_PULLUP);

    if (Board::logic.getRebootRequest())
    {
      Board::logic.blinkLed(255, 165, 0); // Blink orange
      ESP_LOGI(TAG, "Factory reset pending...");
      return;
    }

    if (digitalRead(pins.buttons.factory_defaults_pin) == LOW)
    {
      ESP_LOGI(TAG, "Factory reset button pressed");
      esp_task_wdt_delete(NULL);     // remove current thread from WDT watch (it will be re-added in the next loop()
      openConfigPortal(true, false); // Network configuration setup
      esp_task_wdt_add(NULL);        // add current thread to WDT watch
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
  const Task t_network("Wifi/MQTT", conf::tasks::MQTT_REFRESH_PERIOD, &taskConnect, Board::scheduler, true, conf::tasks::MQTT_REFRESH_PERIOD);
  const Task t_powoff("Poweroff", 1s, &taskPoweroffCheck, Board::scheduler, true);
  const Task t_log("Logoff", 1s, &taskLogoffCheck, Board::scheduler, true);
  // Hardware watchdog will run at one third the frequency
  Task t_wdg("Watchdog", conf::tasks::WATCHDOG_PERIOD, &taskEspWatchdog, Board::scheduler, false);
  const Task t_test("Selftest", conf::tasks::RFID_SELFTEST_PERIOD, &taskRfidWatchdog, Board::scheduler, true);
  const Task t_warn("PoweroffWarning", conf::machine::DELAY_BETWEEN_BEEPS, &taskPoweroffWarning, Board::scheduler, true);
  const Task t_mqtt("MQTT client loop", 1s, &taskMQTTClientLoop, Board::scheduler, true);
  const Task t_led("LED", 1s, &taskBlink, Board::scheduler, true);
  const Task t_rst("FactoryReset", 500ms, &taskFactoryReset, Board::scheduler, pins.buttons.factory_defaults_pin != NO_PIN);
  const Task t_alive("IsAlive", conf::tasks::MQTT_ALIVE_PERIOD, &taskIsAlive, Board::scheduler, true, conf::tasks::MQTT_ALIVE_PERIOD);
#if (RFID_SIMULATION)
  const Task t_sim("RFIDCardsSim", 1s, &taskRFIDCardSim, Board::scheduler, true, 30s);
#endif

  // flag for saving data
  std::atomic<bool> shouldSaveConfig = false;

  // callback notifying us of the need to save config
  void saveConfigCallback()
  {
    shouldSaveConfig.store(true);
  }

  // Called just before the webportal is started after failed WiFi connection
  void configModeCallback(WiFiManager *myWiFiManager)
  {
    ESP_LOGI(TAG, "Entering portal config mode");
    ESP_LOGD(TAG, "%s", WiFi.softAPIP().toString().c_str());
    ESP_LOGD(TAG, "%s", myWiFiManager->getConfigPortalSSID().c_str());
    Board::logic.changeStatus(Status::PortalStarting);
  }

  // Starts the WiFi and possibly open the config portal in a blocking manner
  /// @param force_reset if true, the portal will be reset to factory defaults
  /// @param disable_portal if true, the portal will be disabled (useful at boot-time)
  void openConfigPortal(bool force_reset, bool disable_portal)
  {
    WiFiManager wifiManager;
    SavedConfig config;

    auto opt_settings = SavedConfig::LoadFromEEPROM();
    if (force_reset || !opt_settings)
    {
      config = SavedConfig::DefaultConfig();
    }
    else
    {
      config = opt_settings.value();
    }

    WiFiManagerParameter custom_mqtt_server("Broker", "MQTT Broker address", config.mqtt_server, sizeof(config.mqtt_server));
    WiFiManagerParameter custom_mqtt_topic("Topic", "MQTT Switch topic (leave empty to disable)", config.mqtt_switch_topic, sizeof(config.mqtt_switch_topic));
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

    if (force_reset)
    {
      wifiManager.resetSettings();
    }

#if (PINS_WOKWI)
    wifiManager.setDebugOutput(true);
    wifiManager.resetSettings();
    wifiManager.setTimeout(10); // fail fast for debugging
#endif

    if (disable_portal || config.disablePortal)
    {
      wifiManager.setDisableConfigPortal(true);
    }

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
      strncpy(config.mqtt_switch_topic, custom_mqtt_topic.getValue(), sizeof(config.mqtt_switch_topic));
      strncpy(config.machine_id, custom_machine_id.getValue(), sizeof(config.machine_id));

      config.disablePortal = true;

      // save the custom parameters to EEPROM
      if (config.SaveToEEPROM())
      {
        ESP_LOGD(TAG, "Config saved to EEPROM");
      }
      else
      {
        ESP_LOGE(TAG, "Failed to save config to EEPROM");
      }

      // WiFi settings change may require full reboot
      Board::logic.setRebootRequest(true);
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

  void printCompileSettings()
  {
    using namespace conf;
    std::cout << "Compile-time settings (may be overriden)" << '\n';
    // namespace conf::default_config
    std::cout << "Machine defaults:" << '\n';
    std::cout << "\tmqtt_server: " << default_config::mqtt_server << '\n';
    std::cout << "\tmqtt_switch_topic: " << default_config::mqtt_switch_topic << '\n';
    std::cout << "\tmachine_id: " << default_config::machine_id.id << '\n';
    std::cout << "\tmachine_name: " << default_config::machine_name << '\n';
    std::cout << "\tmachine_type: " << static_cast<int>(default_config::machine_type) << '\n';
    std::cout << "\thostname: " << default_config::hostname << '\n';
    // namespace conf::rfid_tags
    std::cout << "RFID tags:" << '\n';
    std::cout << "\tUID_BYTE_LEN: " << +rfid_tags::UID_BYTE_LEN << '\n';
    std::cout << "\tCACHE_LEN: " << +rfid_tags::CACHE_LEN << '\n';
    // namespace conf::lcd
    std::cout << "LCD config" << '\n';
    std::cout << "\tLCD ROWS: " << +lcd::ROWS << ", COLS: " << +lcd::COLS << '\n';
    std::cout << "\tSHORT_MESSAGE_DELAY: " << std::chrono::milliseconds(lcd::SHORT_MESSAGE_DELAY).count() << "ms" << '\n';
    // namespace conf::machine
    std::cout << "General settings:" << '\n';
    std::cout << "\tDEFAULT_AUTO_LOGOFF_DELAY: " << std::chrono::hours(machine::DEFAULT_AUTO_LOGOFF_DELAY).count() << "h" << '\n';
    std::cout << "\tBEEP_PERIOD: " << std::chrono::seconds(machine::BEEP_PERIOD).count() << "s" << '\n';
    std::cout << "\tDEFAULT_GRACE_PERIOD: " << std::chrono::minutes(machine::DEFAULT_GRACE_PERIOD).count() << "min" << '\n';
    std::cout << "\tDELAY_BETWEEN_BEEPS: " << std::chrono::seconds(machine::DELAY_BETWEEN_BEEPS).count() << "s" << '\n';
    std::cout << "\tMAINTENANCE_BLOCK: " << machine::MAINTENANCE_BLOCK << '\n';
    std::cout << "\tLONG_TAP_DURATION: " << std::chrono::seconds(machine::LONG_TAP_DURATION).count() << "s" << '\n';
    // namespace conf::debug
    std::cout << "Debug settings:" << '\n';
    std::cout << "\tENABLE_LOGS: " << debug::ENABLE_LOGS << '\n';
    std::cout << "\tENABLE_TASK_LOGS: " << debug::ENABLE_TASK_LOGS << '\n';
    std::cout << "\tSERIAL_SPEED_BDS: " << debug::SERIAL_SPEED_BDS << '\n';
    std::cout << "\tFORCE_PORTAL: " << debug::FORCE_PORTAL << '\n';
    std::cout << "\tLOAD_EEPROM_DEFAULTS: " << debug::LOAD_EEPROM_DEFAULTS << '\n';
    // namespace conf::buzzer
    std::cout << "Buzzer settings:" << '\n';
    std::cout << "\tLEDC_PWM_CHANNEL: " << buzzer::LEDC_PWM_CHANNEL << '\n';
    std::cout << "\tSTANDARD_BEEP_DURATION: " << std::chrono::milliseconds(buzzer::STANDARD_BEEP_DURATION).count() << "ms" << '\n';
    std::cout << "\tNB_BEEPS: " << buzzer::NB_BEEPS << '\n';
    std::cout << "\tBEEP_HZ: " << buzzer::BEEP_HZ << '\n';
    // namespace conf::tasks
    std::cout << "Tasks settings:" << '\n';
    std::cout << "\tRFID_CHECK_PERIOD: " << std::chrono::milliseconds(tasks::RFID_CHECK_PERIOD).count() << "ms" << '\n';
    std::cout << "\tRFID_SELFTEST_PERIOD: " << std::chrono::seconds(tasks::RFID_SELFTEST_PERIOD).count() << "s" << '\n';
    std::cout << "\tMQTT_REFRESH_PERIOD: " << std::chrono::seconds(tasks::MQTT_REFRESH_PERIOD).count() << "s" << '\n';
    std::cout << "\tWATCHDOG_TIMEOUT: " << std::chrono::seconds(tasks::WATCHDOG_TIMEOUT).count() << "s" << '\n';
    std::cout << "\tWATCHDOG_PERIOD: " << std::chrono::seconds(tasks::WATCHDOG_PERIOD).count() << "s" << '\n';
    std::cout << "\tPORTAL_CONFIG_TIMEOUT: " << std::chrono::seconds(tasks::PORTAL_CONFIG_TIMEOUT).count() << "s" << '\n';
    std::cout << "\tMQTT_ALIVE_PERIOD: " << std::chrono::seconds(tasks::MQTT_ALIVE_PERIOD).count() << "s" << '\n';
    // namespace conf::mqtt
    std::cout << "MQTT settings:" << '\n';
    std::cout << "\ttopic: " << mqtt::topic << '\n';
    std::cout << "\tresponse_topic: " << mqtt::response_topic << '\n';
    std::cout << "\tMAX_TRIES: " << mqtt::MAX_TRIES << '\n';
    std::cout << "\tTIMEOUT_REPLY_SERVER: " << std::chrono::milliseconds(mqtt::TIMEOUT_REPLY_SERVER).count() << "ms" << '\n';
    std::cout << "\tPORT_NUMBER: " << mqtt::PORT_NUMBER << '\n';
    // Now dump all pins.hpp settings
    std::cout << "Hardware settings:" << '\n';
    std::cout << "\tLED:" << '\n';
    std::cout << "\t\tPin:" << +pins.led.pin << " (G:" << +pins.led.green_pin << ", B:" << +pins.led.blue_pin << ")" << '\n';
    std::cout << "\t\tType is neopixel:" << pins.led.is_neopixel << ", is rgb:" << pins.led.is_rgb << '\n';
    std::cout << "\t\tNeopixel config flags:" << pins.led.neopixel_config << '\n';
    std::cout << "\tMfrc522 chip:" << '\n';
    std::cout << "\t\tSPI settings: MISO: " << +pins.mfrc522.miso_pin << ","
              << " MOSI: " << +pins.mfrc522.mosi_pin << ","
              << " SCK: " << +pins.mfrc522.sck_pin << ","
              << " SDA: " << +pins.mfrc522.sda_pin << '\n';
    std::cout << "\t\tRESET pin:" << +pins.mfrc522.reset_pin << '\n';
    std::cout << "\tLCD module:" << '\n';
    std::cout << "\t\tParallel interface D0:" << +pins.lcd.d0_pin << ", D1:" << +pins.lcd.d1_pin << ", D2:" << +pins.lcd.d2_pin << ", D3:" << +pins.lcd.d3_pin << '\n';
    std::cout << "\t\tReset pin:" << +pins.lcd.rs_pin << ", Enable pin:" << +pins.lcd.en_pin << '\n';
    std::cout << "\t\tBacklight pin:" << +pins.lcd.bl_pin << " (active low:" << pins.lcd.active_low << ")" << '\n';
    std::cout << "\tRelay:" << '\n';
    std::cout << "\t\tControl pin:" << +pins.relay.ch1_pin << " (active low:" << pins.relay.active_low << ")" << '\n';
    std::cout << "\tBuzzer:" << '\n';
    std::cout << "\t\tPin:" << +pins.buzzer.pin << '\n';
    std::cout << "\tButtons:" << '\n';
    std::cout << "\t\tFactory defaults pin:" << +pins.buttons.factory_defaults_pin << '\n';
    std::cout << std::endl;
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
  delay(3000);

  if constexpr (fablabbg::conf::debug::ENABLE_LOGS)
  {
    Serial.setDebugOutput(true);
    ESP_LOGD(TAG, "Starting setup!");
    fablabbg::printCompileSettings();
  }

  if constexpr (fablabbg::conf::debug::LOAD_EEPROM_DEFAULTS)
  {
    auto defaults = fablabbg::SavedConfig::DefaultConfig();
    ESP_LOGW(TAG, "Forcing EEPROM defaults : %s", defaults.toString().c_str());
    defaults.SaveToEEPROM();
  }

  logic.blinkLed();

  // Initialize hardware (RFID, LCD)
  auto hw_init = logic.configure(rfid, lcd);
  hw_init &= logic.initBoard();

  auto count = fablabbg::SavedConfig::IncrementBootCount();
  ESP_LOGI(TAG, "Boot count: %d, reset reason: %d", count, esp_reset_reason());

  logic.changeStatus(Status::Booting);

  if (!hw_init)
  {
    logic.changeStatus(Status::ErrorHardware);
    logic.beepFail();
    logic.blinkLed();
    ESP_LOGE(TAG, "Hardware initialization failed");
  }
  else
  {
    logic.beepOk();
  }

  fablabbg::openConfigPortal(fablabbg::conf::debug::LOAD_EEPROM_DEFAULTS,
                             !fablabbg::conf::debug::FORCE_PORTAL);

#if (MQTT_SIMULATION)
  fablabbg::startMQTTBrocker();
#endif

  fablabbg::setupOTA();

  if (!hw_init)
  {
    // If hardware initialization failed, wait for OTA for 3 minutes
    esp_task_wdt_init(60 * 3, true); // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);          // add current thread to WDT watch
    while (true)
    {
      fablabbg::Tasks::delay(1s);
      ESP_LOGE(TAG, "Hardware failed, waiting for OTA");
    }
  }

  // Let some time for WiFi to settle
  fablabbg::Tasks::delay(2s);

  // Enable the HW watchdog
  fablabbg::t_wdg.enable();
  // Since the WiFiManager may have taken minutes, recompute the tasks schedule
  scheduler.updateSchedules();

  // Try to connect immediately
  fablabbg::taskConnect();
}

void loop()
{
  fablabbg::Board::scheduler.execute();
  ArduinoOTA.handle();
}
#endif // PIO_UNIT_TESTING