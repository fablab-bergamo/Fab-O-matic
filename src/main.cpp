#include <cstdint>
#include <string>
#include <array>

#include <esp_task_wdt.h>
#include "SPIFFS.h"

#include "globals.hpp"
#include "pins.hpp"
#include "BoardLogic.hpp"
#include "pthread.h"

#include "Tasks.hpp"
#include <WiFiManager.h>

namespace fablabbg
{
  using namespace Board;
  using namespace Tasks;
  using Status = BoardLogic::Status;

  /// @brief Opens WiFi and server connection and updates board state accordingly
  void taskConnect()
  {
    if (conf::debug::ENABLE_TASK_LOGS)
    {
      Serial.printf("taskConnect, millis %lu\r\n", millis());
    }

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
    if (conf::debug::ENABLE_TASK_LOGS)
      Serial.println("taskCheckRfid");

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
    if (conf::debug::ENABLE_TASK_LOGS)
      Serial.println("taskPoweroffCheck");

    if (machine.canPowerOff())
    {
      machine.power(false);
    }
  }

  /// @brief periodic check if the machine must be powered off
  void taskPoweroffWarning()
  {
    if (conf::debug::ENABLE_TASK_LOGS)
      Serial.println("taskPoweroffWarning");

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
    if (conf::debug::ENABLE_TASK_LOGS)
      Serial.println("taskLogoffCheck");

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

    if (conf::debug::ENABLE_TASK_LOGS)
      Serial.println("taskEspWatchdog");

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
    if (conf::debug::ENABLE_TASK_LOGS)
      Serial.println("taskRfidWatchdog");

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
    if (conf::debug::ENABLE_TASK_LOGS)
      Serial.println("taskMQTTAlive");

    if (server.isOnline())
    {
      server.loop();
    }
  }

  void taskLcdRefresh()
  {
    if (conf::debug::ENABLE_TASK_LOGS)
      Serial.println("taskLcdRefresh");

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

    delay(3000);
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
  // The callback will be executed in the same thread as the scheduler

  Task t1("RFIDChip", conf::tasks::RFID_CHECK_PERIOD, &taskCheckRfid, scheduler, true);
  Task t2("Wifi/MQQT init", conf::tasks::MQTT_REFRESH_PERIOD, &taskConnect, scheduler, true);
  Task t3("Poweroff", std::chrono::seconds(1), &taskPoweroffCheck, scheduler, true);
  Task t4("Logoff", std::chrono::seconds(1), &taskLogoffCheck, scheduler, true);
  // Hardware watchdog will run at half the frequency
  Task t5("Watchdog", conf::tasks::WATCHDOG_TIMEOUT / 2, &taskEspWatchdog, scheduler, true);
  Task t6("Selftest", conf::tasks::RFID_SELFTEST_PERIOD, &taskRfidWatchdog, scheduler, true);
  Task t7("PoweroffWarning", conf::machine::DELAY_BETWEEN_BEEPS, &taskPoweroffWarning, scheduler, true);
  Task t8("MQTT keepalive", std::chrono::seconds(1), &taskMQTTAlive, scheduler, true);
  Task t9("LED", std::chrono::seconds(1), &taskBlink, scheduler, true);
  // Wokwi requires LCD refresh unlike real hardware
  Task t10("LCDRefresh", std::chrono::seconds(2), &taskLcdRefresh, scheduler, false);

} // namespace fablabbg

using namespace fablabbg;

void config_portal()
{
  char mqtt_server[40]{0};

  WiFiManager wifiManager;

#ifdef DEBUG
  wifiManager.setDebugOutput(true);
  wifiManager.resetSettings();
#endif

  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_mqtt_server("MQTT", "MQTT Broker address", mqtt_server, 40);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.setTimeout(240);       // 4 minutes for configuration
  wifiManager.setConnectRetries(3);  // 3 retries
  wifiManager.setConnectTimeout(10); // 10 seconds
  wifiManager.setCountry("IT");
  wifiManager.setTitle("FabLab Bergamo - RFID arduino");
  wifiManager.setCaptivePortalEnable(true);
  wifiManager.autoConnect();
}

void setup()
{
  Serial.begin(conf::debug::SERIAL_SPEED_BDS); // Initialize serial communications with the PC for debugging.

  if (conf::debug::ENABLE_LOGS)
  {
    Serial.println("Starting setup!");
    Serial.println(machine.toString().c_str());
  }

  config_portal();

  if (!logic.board_init())
  {
    logic.changeStatus(Status::ERROR_HW);
    if constexpr (pins.led.is_neopixel)
    {
      logic.set_led_color(255, 0, 0);
      logic.led(true);
    }
    logic.beep_failed();
    while (true)
      ;
  }

#if (WOKWI_SIMULATION)
  attr_mqtt_broker.stacksize = 3 * 1024;
  attr_mqtt_broker.detachstate = PTHREAD_CREATE_DETACHED;
  if (pthread_create(&thread_mqtt_broker, &attr_mqtt_broker, threadMQTTServer, NULL))
  {
    Serial.println("Error creating MQTT server thread");
  }
  t10.start();
#endif

  logic.beep_ok();
  server.connectWiFi();
  // Don't try to connect immediately, restart the 30s now
  t2.restart();
}

void loop()
{
  scheduler.execute();
}
