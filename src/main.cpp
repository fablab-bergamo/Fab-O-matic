#include "pthread.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include "BoardLogic.hpp"
#include "Espressif.hpp"
#include "Logging.hpp"
#include "Mrfc522Driver.hpp"
#include "SavedConfig.hpp"
#include "Tasks.hpp"
#include "globals.hpp"
#if (MQTT_SIMULATION)
#include "mock/MockMQTTBroker.hpp"
#endif
#include "pins.hpp"
#include "language/lang.hpp"
#include "OTA.hpp"

using namespace std::chrono_literals;

namespace fabomatic
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

      if (server.isOnline())
      {
        // Briefly show to the user
        Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY);
      }
    }

    if (server.isOnline())
    {
      ESP_LOGI(TAG, "taskConnect - online, calling refreshFromServer");
      // Get machine data from the server if it is online
      Board::logic.refreshFromServer();
      if (auto &machine = Board::logic.getMachine(); !machine.isFree())
      {
        const auto response = Board::logic.getServer().inUse(
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
        esp32::restart();
      }
    }
  }

  /// @brief periodic check if the user shall be logged off
  void taskLogoffCheck()
  {
    // auto logout after delay
    const auto &machine = Board::logic.getMachine();
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
        initialized = esp32::setupWatchdog(conf::tasks::WATCHDOG_TIMEOUT);
      }
      if (initialized)
      {
        if (!esp32::signalWatchdog())
        {
          ESP_LOGE(TAG, "Failure to signal watchdog");
        }
      }
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

    if (!server.saveBuffer())
    {
      ESP_LOGE(TAG, "Failure to save buffered MQTT messages");
    }

    if constexpr (conf::debug::ENABLE_LOGS)
    {
      fabomatic::esp32::showHeapStats();
    }
  }

  void taskFactoryReset()
  {
    if constexpr (pins.buttons.factory_defaults_pin == NO_PIN)
    {
      return;
    }

    // Skip factory reset for this specific board because Factory Reset is soldered under the MCU with the reset pin.
    if (const auto &serial = esp32::esp_serial(); serial == "dcda0c419794")
    {
      pinMode(pins.buttons.factory_defaults_pin, INPUT); // Disable pull-up/pull-downs
      return;
    }

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
      esp32::removeWatchdog();
      openConfigPortal(true, false); // Network configuration setup
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
        const auto [card_uid, level, name] = secrets::cards::whitelist[random(0, secrets::cards::whitelist.size())];
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
    // simulation constants
    std::cout << "Compilation settings" << '\n';
    std::cout << "\tMQTT_SIMULATION: " << MQTT_SIMULATION << '\n';
    std::cout << "\tRFID_SIMULATION: " << RFID_SIMULATION << '\n';
    std::cout << "\tCORE_DEBUG_LEVEL: " << CORE_DEBUG_LEVEL << '\n';
    std::cout << "\tLANGUAGE: " << fabomatic::strings::S_LANG_ID << '\n';
    std::cout << "\tBUILD: " << FABOMATIC_BUILD << '\n';

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
    std::cout << "\tDEFAULT_GRACE_PERIOD: " << std::chrono::seconds(machine::DEFAULT_GRACE_PERIOD).count() << "s" << '\n';
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
    std::cout << "\tSTANDARD_BEEP_DURATION: " << std::chrono::milliseconds(buzzer::STANDARD_BEEP_DURATION).count() << "ms" << '\n';
    std::cout << "\tNB_BEEPS: " << buzzer::NB_BEEPS << '\n';
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
} // namespace fabomatic

#ifndef PIO_UNIT_TESTING
void setup()
{
  using Status = fabomatic::BoardLogic::Status;
  auto &logic = fabomatic::Board::logic;
  auto &scheduler = fabomatic::Board::scheduler;
  auto &rfid = fabomatic::Board::rfid;
  auto &lcd = fabomatic::Board::lcd;

  Serial.begin(fabomatic::conf::debug::SERIAL_SPEED_BDS); // Initialize serial communications with the PC for debugging.
  delay(3000);

  esp_log_level_set(TAG, LOG_LOCAL_LEVEL);

  if constexpr (fabomatic::conf::debug::ENABLE_LOGS)
  {
    Serial.setDebugOutput(true);
    ESP_LOGD(TAG, "Starting setup!");
    fabomatic::printCompileSettings();
  }

  if constexpr (fabomatic::conf::debug::LOAD_EEPROM_DEFAULTS)
  {
    const auto &defaults = fabomatic::SavedConfig::DefaultConfig();
    ESP_LOGW(TAG, "Forcing EEPROM defaults : %s", defaults.toString().c_str());
    defaults.SaveToEEPROM();
  }

  logic.blinkLed();

  // Initialize hardware (RFID, LCD)
  auto hw_init = logic.configure(rfid, lcd);
  hw_init &= logic.initBoard();

  const auto count = fabomatic::SavedConfig::IncrementBootCount();
  ESP_LOGI(TAG, "Boot count: %d, reset reason: %d", count, esp_reset_reason());

  logic.changeStatus(Status::Booting);

  if (!hw_init)
  {
    logic.changeStatus(Status::ErrorHardware);
    logic.beepFail();
    logic.blinkLed(64, 0, 0);
    ESP_LOGE(TAG, "Hardware initialization failed");
  }
  else
  {
    logic.beepOk();
    logic.blinkLed(0, 64, 0);
  }

  fabomatic::openConfigPortal(fabomatic::conf::debug::LOAD_EEPROM_DEFAULTS,
                              !fabomatic::conf::debug::FORCE_PORTAL);

#if (MQTT_SIMULATION)
  fabomatic::startMQTTBrocker();
#endif

  fabomatic::setupOTA();

  if (!hw_init)
  {
    // If hardware initialization failed, wait for OTA for 3 minutes
    if (!fabomatic::esp32::setupWatchdog(180s))
    {
      ESP_LOGE(TAG, "Failed to setup watchdog!");
    }
    while (true)
    {
      logic.blinkLed(64, 0, 0);
      logic.changeStatus(fabomatic::BoardLogic::Status::ErrorHardware);
      fabomatic::Tasks::delay(1s);
      ESP_LOGE(TAG, "Hardware failed, waiting for OTA");
    }
  }

  // Let some time for WiFi to settle
  fabomatic::Tasks::delay(2s);

  // Enable the HW watchdog
  fabomatic::t_wdg.enable();
  // Since the WiFiManager may have taken minutes, recompute the tasks schedule
  scheduler.updateSchedules();

  // Try to connect immediately
  fabomatic::taskConnect();
}

void loop()
{
  fabomatic::Board::scheduler.execute();
  ArduinoOTA.handle();
}
#endif // PIO_UNIT_TESTING