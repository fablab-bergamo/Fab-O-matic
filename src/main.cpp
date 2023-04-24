#include <cstdint>
#include <string>
#include <array>

#include <esp_task_wdt.h>
#include <TaskScheduler.h>

#include "globals.h"
#include "pins.h"
#include "BoardLogic.h"

// Pre-declarations
void taskCheckRfid();
void taskConnect();
void taskPoweroffCheck();
void taskLogoffCheck();
void taskEspWatchdog();
void taskRfidWatchdog();
void taskPoweroffWarning();

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

const Task t1(conf::tasks::RFID_CHECK_MS *TASK_MILLISECOND, TASK_FOREVER, &taskCheckRfid, &Tasks::ts, true);
const Task t2(conf::tasks::REFRESH_PERIOD_SECONDS *TASK_SECOND, TASK_FOREVER, &taskConnect, &Tasks::ts, true);
const Task t3(1 * TASK_SECOND, TASK_FOREVER, &taskPoweroffCheck, &Tasks::ts, true);
const Task t4(1 * TASK_SECOND, TASK_FOREVER, &taskLogoffCheck, &Tasks::ts, true);

// Hardware watchdog will run at half the frequency
const Task t5(conf::tasks::WDG_TIMEOUT_S / 2 * TASK_SECOND, TASK_FOREVER, &taskEspWatchdog, &Tasks::ts, true);
const Task t6(conf::tasks::RFID_CHIP_CHECK_S *TASK_SECOND, TASK_FOREVER, &taskRfidWatchdog, &Tasks::ts, true);
const Task t7(conf::machine::DELAY_BETWEEN_BEEPS_S *TASK_SECOND, TASK_FOREVER, &taskPoweroffWarning, &Tasks::ts, true);

// NOLINTEND(cert-err58-cpp)

using namespace Board;
using Status = BoardLogic::Status;

/// @brief Opens WiFi and server connection and updates board state accordingly
void taskConnect()
{
  if (conf::debug::ENABLE_LOGS)
    Serial.println("Trying Wifi and server connection...");

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
  if (conf::machine::TIMEOUT_USAGE_MINUTES > 0 &&
      machine.getUsageTime() > conf::machine::TIMEOUT_USAGE_MINUTES * 60 * 1000)
  {
    Serial.println("Auto-logging out user");
    logic.logout();
  }
}

/// @brief Keep the ESP32 HW watchdog alive.
/// If code gets stuck this will cause an automatic reset.
void taskEspWatchdog()
{
  if (conf::tasks::WDG_TIMEOUT_S > 0)
  {
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
      delay(conf::tasks::RFID_CHECK_MS);
  }
}

void setup()
{
  Serial.begin(conf::debug::SERIAL_SPEED_BDS); // Initialize serial communications with the PC for debugging.

  if (conf::debug::ENABLE_LOGS)
    Serial.println("Starting setup!");

  if (conf::tasks::WDG_TIMEOUT_S > 0)
  {
    esp_task_wdt_init(conf::tasks::WDG_TIMEOUT_S, true); // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);                              // add current thread to WDT watch
  }

  if (!logic.init())
  {
    logic.changeStatus(Status::ERROR);
    logic.beep_failed();
    while (true)
      ;
  }
  logic.beep_ok();
}

void loop()
{
  Tasks::ts.execute();
}