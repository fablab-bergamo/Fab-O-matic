#include "BoardLogic.h"
#include "Arduino.h"
#include "secrets.h"
#include "conf.h"
#include "Machine.h"
#include "FabServer.h"
#include "RFIDWrapper.h"
#include "MockRFIDWrapper.h"
#include "AuthProvider.h"
#include <esp_task_wdt.h>
#include "LCDWrapper.h"
#include <sstream>

namespace Board
{
  // Only main.cpp instanciates the variables through Board.h file
#ifdef WOKWI_SIMULATION
  extern MockRFIDWrapper rfid;
#else
  extern RFIDWrapper rfid;
#endif
  extern FabServer server;
  extern Machine machine;
  extern AuthProvider auth;
  extern LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd;
} // namespace Board

BoardLogic::BoardLogic() noexcept : status(Status::CLEAR) {}

/// @brief connects and polls the server for up-to-date machine information
void BoardLogic::refreshFromServer()
{
  if (Board::server.connect())
  {
    // Check the configured machine data from the server
    auto prevStatus = this->getStatus();

    this->changeStatus(Status::CONNECTING);
    auto result = Board::server.checkMachine(secrets::machine::machine_id);
    this->changeStatus(prevStatus);

    if (result->request_ok)
    {
      if (result->is_valid)
      {
        if (conf::debug::ENABLE_LOGS)
          Serial.printf("The configured machine ID %d is valid, maintenance=%d, allowed=%d\r\n",
                        secrets::machine::machine_id, result->needs_maintenance, result->allowed);

        Board::machine.maintenanceNeeded = result->needs_maintenance;
        Board::machine.allowed = result->allowed;
      }
      else
      {
        Serial.printf("The configured machine ID %d is unknown to the server\r\n",
                      secrets::machine::machine_id);
      }
    }
  }
}

/// @brief Called when a RFID tag has been detected
void BoardLogic::onNewCard()
{
  if (conf::debug::ENABLE_LOGS)
    Serial.println("New card present");

  // if there is a "new" card (could be the same that stayed in the field)
  if (!this->ready_for_a_new_card || !Board::rfid.readCardSerial())
  {
    return;
  }
  this->ready_for_a_new_card = false;

  // Acquire the UID of the card
  auto uid = Board::rfid.getUid();

  if (Board::machine.isFree())
  {
    // machine is free
    if (!this->authorize(uid))
    {
      Serial.println("Login failed");
    }
    delay(1000);
    this->refreshFromServer();
    return;
  }

  // machine is busy
  if (Board::machine.getActiveUser().card_uid == uid)
  {
    // we can logout. we should require that the card stays in the field for some seconds, to prevent accidental logout. maybe sound a buzzer?
    this->logout();
  }
  else
  {
    // user is not the same, display who is using it
    this->changeStatus(Status::ALREADY_IN_USE);
  }
  delay(1000);
  return;
}

/// @brief Removes the current machine user and changes the status to LOGOUT
void BoardLogic::logout()
{
  auto result = Board::server.finishUse(Board::machine.getActiveUser().card_uid,
                                        Board::machine.getMachineId(),
                                        Board::machine.getUsageTime() / 1000U);

  if (conf::debug::ENABLE_LOGS)
    Serial.printf("Result finishUse: %d\r\n", result->request_ok);

  Board::machine.logout();
  this->user = FabUser();
  this->changeStatus(Status::LOGOUT);
  this->beep_ok();
  delay(1000);
}

/// @brief Asks the user to keep the RFID tag on the reader as confirmation
/// @return True if the user accepted, False if user bailed out
bool BoardLogic::longTap(std::string_view short_prompt) const
{
  constexpr unsigned int TOTAL_DURATION_MS = 3000;
  constexpr unsigned int STEPS_COUNT = 6;
  constexpr unsigned int STEP_DELAY = (TOTAL_DURATION_MS / STEPS_COUNT);

  const BoardInfo bi = {Board::server.isOnline(), Board::machine.getPowerState(), Board::machine.isShutdownImminent()};

  bool user_bailed_out = false;
  for (auto step = 0; step < STEPS_COUNT; step++)
  {
    std::stringstream ss;
    ss << short_prompt << " " << step << "/" << STEPS_COUNT;
    Board::lcd.setRow(1, ss.str());
    Board::lcd.update_chars(bi);

    delay(STEP_DELAY);

    if (!Board::rfid.cardStillThere(this->user.card_uid))
    {
      Board::lcd.setRow(1, "");
      Board::lcd.update_chars(bi);
      return false;
    }
  }

  Board::lcd.setRow(1, "");
  Board::lcd.update_chars(bi);
  return true;
}

/// @brief Checks if the card UID is valid, and tries to check the user in to the machine.
/// @param uid card uid
/// @return true if the user is now logged on to the machine
bool BoardLogic::authorize(card::uid_t uid)
{
  this->changeStatus(Status::VERIFYING);
  this->user.authenticated = false;
  this->user.holder_name = "?";
  this->user.card_uid = uid;
  this->user.user_level = FabUser::UserLevel::UNKNOWN;

  auto response = Board::auth.tryLogin(uid);
  if (!response.has_value())
  {
    Serial.println("Failed login");
    this->changeStatus(Status::LOGIN_DENIED);
    this->beep_failed();
    return false;
  }

  this->user = response.value();

  if (!Board::machine.allowed)
  {
    Serial.println("Machine blocked");
    this->changeStatus(Status::NOT_ALLOWED);
    this->beep_failed();
    return false;
  }

  if (Board::machine.maintenanceNeeded)
  {
    if (conf::machine::MAINTENANCE_BLOCK &&
        this->user.user_level < FabUser::UserLevel::FABLAB_ADMIN)
    {
      this->changeStatus(Status::MAINTENANCE_NEEDED);
      this->beep_failed();
      delay(3000);
      return false;
    }
    if (this->user.user_level >= FabUser::UserLevel::FABLAB_ADMIN)
    {
      this->beep_ok();
      this->changeStatus(Status::MAINTENANCE_QUERY);

      if (this->longTap("Registra"))
      {
        auto response = Board::server.registerMaintenance(this->user.card_uid, Board::machine.getMachineId());
        if (!response->request_ok)
        {
          this->beep_failed();
          this->changeStatus(Status::ERROR);
          delay(1000);
        }
        else
        {
          this->beep_ok();
          this->changeStatus(Status::MAINTENANCE_DONE);
          delay(1000);
        }
        // Proceed to log-on the staff member to the machine in all cases
      }
    }
  }
  Board::machine.login(this->user);
  auto result = Board::server.startUse(Board::machine.getActiveUser().card_uid,
                                       Board::machine.getMachineId());

  if (conf::debug::ENABLE_LOGS)
    Serial.printf("Result startUse: %d\r\n", result->request_ok);

  this->changeStatus(Status::LOGGED_IN);
  this->beep_ok();
  return true;
}

/// @brief Initializes LCD and RFID classes
bool BoardLogic::init()
{
  if (conf::debug::ENABLE_LOGS)
    Serial.println("Initializing board...");

  bool success = Board::lcd.begin();
  success &= Board::rfid.init();

  // Setup buzzer pin for ESP32
  success &= (ledcSetup(conf::buzzer::LEDC_PWM_CHANNEL, conf::buzzer::BEEP_HZ, 10U) != 0);
  ledcAttachPin(pins.buzzer.pin, conf::buzzer::LEDC_PWM_CHANNEL);

  // Force Wifi disconnect as ESP32 has persistent wifi config
  WiFi.disconnect();
  success &= WiFi.mode(WIFI_STA);

  if (conf::debug::ENABLE_LOGS)
    Serial.printf("Board init complete, success = %d\r\n", success);

  return success;
}

/// @brief Sets the board in the state given.
/// @param new_state new state
void BoardLogic::changeStatus(Status new_state)
{
  if (conf::debug::ENABLE_LOGS && this->status != new_state)
  {
    char buffer[32] = {0};
    if (sprintf(buffer, "** Changing board state to %d", static_cast<int>(new_state)) > 0)
      Serial.println(buffer);
  }

  this->status = new_state;
  this->updateLCD();
}

/// @brief Updates the LCD screen as per the current status
void BoardLogic::updateLCD() const
{
  char buffer[conf::lcd::COLS + 1] = {0}; // Null terminated strings
  std::string user_name, machine_name, uid_str;

  Board::lcd.showConnection(true);
  Board::lcd.showPower(true);

  user_name = Board::machine.getActiveUser().holder_name;
  machine_name = Board::machine.getMachineName();
  uid_str = card::uid_str(this->user.card_uid);

  switch (this->status)
  {
  case Status::CLEAR:
    Board::lcd.clear();
    break;
  case Status::FREE:
    Board::lcd.setRow(0, machine_name);
    if (Board::machine.maintenanceNeeded)
    {
      Board::lcd.setRow(1, ">Manutenzione<");
    }
    else if (!Board::machine.allowed)
    {
      Board::lcd.setRow(1, "> BLOCCATA <");
    }
    else
    {
      Board::lcd.setRow(1, "Avvicina carta");
    }
    break;
  case Status::ALREADY_IN_USE:
    Board::lcd.setRow(0, "In uso da");
    Board::lcd.setRow(1, Board::machine.getActiveUser().holder_name);
    break;
  case Status::LOGGED_IN:
    Board::lcd.setRow(0, "Inizio uso");
    Board::lcd.setRow(1, this->user.holder_name);
    break;
  case Status::LOGIN_DENIED:
    Board::lcd.setRow(0, "Carta ignota");
    Board::lcd.setRow(1, uid_str.data());
    break;
  case Status::LOGOUT:
    Board::lcd.setRow(0, "Arrivederci");
    Board::lcd.setRow(1, user_name);
    break;
  case Status::CONNECTING:
    Board::lcd.setRow(0, "Connessione");
    Board::lcd.setRow(1, "al server...");
    break;
  case Status::CONNECTED:
    Board::lcd.setRow(0, "Connesso");
    Board::lcd.setRow(1, "");
    break;
  case Status::IN_USE:
    if (snprintf(buffer, sizeof(buffer), "Ciao %s", user_name.data()) > 0)
      Board::lcd.setRow(0, buffer);
    Board::lcd.setRow(1, Board::lcd.convertSecondsToHHMMSS(Board::machine.getUsageTime()));
    break;
  case Status::BUSY:
    Board::lcd.setRow(0, "Elaborazione...");
    Board::lcd.setRow(1, "");
    break;
  case Status::OFFLINE:
    Board::lcd.setRow(0, "OFFLINE MODE");
    Board::lcd.setRow(1, "");
    break;
  case Status::NOT_ALLOWED:
    Board::lcd.setRow(0, "Blocco");
    Board::lcd.setRow(1, "amministrativo");
    break;
  case Status::VERIFYING:
    Board::lcd.setRow(0, "VERIFICA IN");
    Board::lcd.setRow(1, "CORSO");
    break;
  case Status::MAINTENANCE_NEEDED:
    Board::lcd.setRow(0, "Blocco per");
    Board::lcd.setRow(1, "manutenzione");
    break;
  case Status::MAINTENANCE_QUERY:
    Board::lcd.setRow(0, "Manutenzione?");
    Board::lcd.setRow(1, "Registra");
    break;
  case Status::MAINTENANCE_DONE:
    Board::lcd.setRow(0, "Manutenzione");
    Board::lcd.setRow(1, "registrata");
    break;
  case Status::ERROR:
    Board::lcd.setRow(0, "Errore");
    Board::lcd.setRow(1, "");
    break;
  default:
    Board::lcd.setRow(0, "Unhandled status");
    if (sprintf(buffer, "Value %d", static_cast<int>(this->status)) > 0)
      Board::lcd.setRow(1, buffer);
    break;
  }
  BoardInfo bi = {Board::server.isOnline(), Board::machine.getPowerState(), Board::machine.isShutdownImminent()};
  Board::lcd.update_chars(bi);
}

/// @brief Gets the current board status
/// @return board status
BoardLogic::Status BoardLogic::getStatus() const
{
  return this->status;
}

/// @brief Gets the latest user acquired by RFID card
/// @return a user object
FabUser BoardLogic::getUser()
{
  return this->user;
}

void BoardLogic::beep_ok() const
{
  ledcWriteTone(conf::buzzer::LEDC_PWM_CHANNEL, conf::buzzer::BEEP_HZ);
  delay(conf::buzzer::BEEP_DURATION_MS);
  ledcWrite(conf::buzzer::LEDC_PWM_CHANNEL, 0UL);
}

void BoardLogic::beep_failed() const
{
  constexpr auto NB_BEEPS = 3;
  for (auto i = 0; i < NB_BEEPS; i++)
  {
    ledcWriteTone(conf::buzzer::LEDC_PWM_CHANNEL, conf::buzzer::BEEP_HZ);
    delay(conf::buzzer::BEEP_DURATION_MS);
    ledcWrite(conf::buzzer::LEDC_PWM_CHANNEL, 0UL);
    delay(conf::buzzer::BEEP_DURATION_MS);
  }
}
