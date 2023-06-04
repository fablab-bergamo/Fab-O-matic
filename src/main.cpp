#include <cstdint>
#include <string>
#include <array>

#include <esp_task_wdt.h>
#include "globals.h"
#include "pins.h"
#include "BoardLogic.h"
#include "pthread.h"

#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>

using namespace Board;
using Status = BoardLogic::Status;

// Pre-declarations
void taskCheckRfid();
void taskConnect();
void taskPoweroffCheck();
void taskLogoffCheck();
void taskEspWatchdog();
void taskRfidWatchdog();
void taskPoweroffWarning();
void taskMQTTAlive();
void taskBlink();

/* This is the list of tasks that will be run in cooperative scheduling fashion.
 * Using tasks simplifies the code versus a single state machine / if (millis() - memory) patterns
 *
 * See https://github.com/arkhipenko/TaskScheduler/blob/master/examples/Scheduler_template/Scheduler_template.ino
 *
 * Task are created as follows :
 * - Task( period ; # repeats ; callback ; scheduler object ; true if task active )
 *
 * Do not add code in loop() as ts.execute() runs forever.
 */

// NOLINTBEGIN(cert-err58-cpp)

const Task t1(duration_cast<milliseconds>(conf::tasks::RFID_CHECK_PERIOD).count() * TASK_MILLISECOND,
              TASK_FOREVER,
              &taskCheckRfid,
              &Tasks::ts, true);

Task t2(duration_cast<milliseconds>(conf::tasks::MQTT_REFRESH_PERIOD).count() * TASK_MILLISECOND,
        TASK_FOREVER,
        &taskConnect,
        &Tasks::ts, true);

const Task t3(1 * TASK_SECOND, TASK_FOREVER,
              &taskPoweroffCheck,
              &Tasks::ts, true);

const Task t4(1 * TASK_SECOND, TASK_FOREVER,
              &taskLogoffCheck,
              &Tasks::ts, true);

// Hardware watchdog will run at half the frequency
const Task t5(duration_cast<milliseconds>(conf::tasks::WATCHDOG_TIMEOUT).count() / 2 * TASK_MILLISECOND,
              TASK_FOREVER,
              &taskEspWatchdog,
              &Tasks::ts, true);

const Task t6(duration_cast<milliseconds>(conf::tasks::RFID_SELFTEST_PERIOD).count() * TASK_MILLISECOND,
              TASK_FOREVER,
              &taskRfidWatchdog,
              &Tasks::ts, true);

const Task t7(duration_cast<milliseconds>(conf::machine::DELAY_BETWEEN_BEEPS).count() * TASK_MILLISECOND,
              TASK_FOREVER,
              &taskPoweroffWarning,
              &Tasks::ts, true);

const Task t8(1 * TASK_SECOND,
              TASK_FOREVER,
              &taskMQTTAlive,
              &Tasks::ts, true);

const Task t9(1 * TASK_SECOND,
              TASK_FOREVER,
              &taskBlink,
              &Tasks::ts, true);

// NOLINTEND(cert-err58-cpp)

/// @brief Opens WiFi and server connection and updates board state accordingly
void taskConnect()
{
  if (conf::debug::ENABLE_TASK_LOGS)
    Serial.println("taskConnect");

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
  static bool value = false;

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
  if (conf::machine::AUTO_LOGOFF_DELAY > 0s &&
      machine.getUsageDuration() > conf::machine::AUTO_LOGOFF_DELAY)
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
  static bool initialized = false;

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

#ifdef WOKWI_SIMULATION
pthread_t mqtt_server;
pthread_attr_t attr;

void *threadMQTTServer(void *arg)
{
  if (conf::debug::ENABLE_LOGS)
    Serial.println("threadMQTTServer started");

  delay(5000);
  bool value = false;
  while (true)
  {
    // Check if the server is online
    if (!Board::broker.isRunning())
    {
      // Try to connect
      Board::broker.start();
    }
    Board::broker.update();
    delay(500);
  }
}
#endif

void setup()
{
  Serial.begin(conf::debug::SERIAL_SPEED_BDS); // Initialize serial communications with the PC for debugging.

  if (conf::debug::ENABLE_LOGS)
    Serial.println("Starting setup!");

  if (!logic.init())
  {
    logic.changeStatus(Status::ERROR);
    logic.beep_failed();
    while (true)
      ;
  }

#ifdef WOKWI_SIMULATION
  attr.stacksize = 3 * 1024;
  attr.detachstate = PTHREAD_CREATE_DETACHED;
  if (pthread_create(&mqtt_server, &attr, threadMQTTServer, NULL))
  {
    Serial.println("Error creating MQTT server thread");
  }
#endif
  logic.beep_ok();
  server.connectWiFi();
  t2.delay(5 * TASK_SECOND);
}

void loop()
{
  Tasks::ts.execute();
}