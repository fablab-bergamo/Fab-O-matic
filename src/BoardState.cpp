#include <string>
#include <cstdint>

#include "BoardState.h"
#include "conf.h"
#include "RFIDWrapper.h"
#include "LCDWrapper.h"
#include "FabServer.h"
#include "Machine.h"
#include "AuthProvider.h"
#include "secrets.h"
#include "pins.h"

namespace Board
{
    // Only main.cpp instanciates the variables through Board.h file
    extern RFIDWrapper rfid;
    extern LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd;
    extern FabServer server;
    extern Machine machine;
    extern AuthProvider auth;
}

/// @brief Initializes LCD and RFID classes
bool BoardState::init()
{
    
    Serial.println("Initializing LCD...");
    bool success = Board::lcd.begin();
    delay(100);
    Serial.println("Initializing RFID...");
    success &= Board::rfid.init();
    delay(100);
    // Setup buzzer pin for ESP32
    success &= (ledcSetup(BoardState::LEDC_CHANNEL, 660U, 10U) != 0);
    ledcAttachPin(pins.buzzer.buzzer_pin, BoardState::LEDC_CHANNEL);

    Serial.printf("Board init complete, success = %d\n", success);
    return success;
}

/// @brief Sets the board in the state given.
/// @param new_state new state
void BoardState::changeStatus(Status new_state)
{
    if (this->status != new_state)
    {
        char buffer[32] = {0};
        sprintf(buffer, "** Changing board state to %d", static_cast<typename std::underlying_type<Status>::type>(new_state));
        Serial.println(buffer);
    }

    this->status = new_state;
    this->update();
}

/// @brief Updates the LCD screen as per the current status
void BoardState::update()
{
    char buffer[conf::lcd::COLS];
    std::string user_name, machine_name, uid_str;

    Board::lcd.showConnection(true);
    Board::lcd.showPower(true);

    user_name = Board::machine.getActiveUser().holder_name;
    machine_name = Board::machine.getMachineName();
    uid_str = card::uid_str(this->member.card_uid);

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
        Board::lcd.setRow(1, this->member.holder_name);
        break;
    case Status::LOGIN_DENIED:
        Board::lcd.setRow(0, "Carta ignota");
        Board::lcd.setRow(1, uid_str.c_str());
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
        snprintf(buffer, sizeof(buffer), "Ciao %s", user_name.c_str());
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
        Board::lcd.setRow(0, "Registrare");
        Board::lcd.setRow(1, "manutenzione?");
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
        sprintf(buffer, "Value %d", static_cast<typename std::underlying_type<Status>::type>(this->status));
        Board::lcd.setRow(1, buffer);
        break;
    }
    BoardInfo bi = {Board::server.isOnline(), Board::machine.getPowerState(), Board::machine.shutdownWarning()};
    Board::lcd.update_chars(bi);
}

/// @brief Checks if the card UID is valid, and tries to check the user in to the machine.
/// @param uid card uid
/// @return true if the user is now logged on to the machine
bool BoardState::authorize(card::uid_t uid)
{
    this->changeStatus(Status::VERIFYING);
    if (Board::auth.tryLogin(uid, this->member))
    {
        if (Board::machine.allowed)
        {
            if (Board::machine.maintenanceNeeded) 
            {
                if (conf::machine::MAINTENANCE_BLOCK && member.user_level < FabUser::UserLevel::FABLAB_ADMIN)
                {
                    this->changeStatus(Status::MAINTENANCE_NEEDED);
                    this->beep_failed();
                    return false;
                }
                if (member.user_level >= FabUser::UserLevel::FABLAB_ADMIN)
                {
                    this->beep_ok();
                    this->changeStatus(Status::MAINTENANCE_QUERY);
                    // User must leave the card for 3s before it's recognized
                    delay(3000);

                    if (Board::rfid.ReadCardSerial() && Board::rfid.GetUid() == member.card_uid)
                    {
                        auto response = Board::server.registerMaintenance(member.card_uid, Board::machine.getMachineId());
                        if (response.request_ok)
                        {
                            this->beep_ok();
                            this->changeStatus(Status::MAINTENANCE_DONE);
                            delay(1000);
                        }
                        else
                        {
                            this->beep_failed();
                            this->changeStatus(Status::ERROR);
                            return false;
                        }
                    }
                }
            }
            Board::machine.login(member);
            auto result = Board::server.startUse(Board::machine.getActiveUser().card_uid, Board::machine.getMachineId());
            Serial.printf("Result startUse: %d\n", result.request_ok);
            this->member = member;
            this->changeStatus(Status::LOGGED_IN);
            this->beep_ok();
            return true;
        }
        this->changeStatus(Status::NOT_ALLOWED);
        this->beep_failed();
        return false;
    }
    else
    {
        Serial.println("Failed login");
    }
    this->changeStatus(Status::LOGIN_DENIED);
    this->beep_failed();
    return false;
}

/// @brief Removes the current machine user and changes the status to LOGOUT
void BoardState::logout()
{
    auto result = Board::server.finishUse(Board::machine.getActiveUser().card_uid, Board::machine.getMachineId(), Board::machine.getUsageTime());
    Serial.printf("Result finishUse: %d\n", result.request_ok);
    Board::machine.logout();
    this->member = FabUser();
    this->changeStatus(Status::LOGOUT);
    this->beep_ok();
    delay(1000);
}

/// @brief Gets the current board status
/// @return board status
BoardState::Status BoardState::getStatus() const
{
    return this->status;
}

/// @brief Gets the latest user acquired by RFID card
/// @return a user object
FabUser BoardState::getUser()
{
    return this->member;
}

void BoardState::beep_ok()
{
    ledcWriteTone(BoardState::LEDC_CHANNEL, 660UL);
    delay(BoardState::BEEP_DURATION_MS);
    ledcWrite(BoardState::LEDC_CHANNEL, 0UL);
}

void BoardState::beep_failed()
{
    constexpr auto NB_BEEPS = 3;
    for (auto i = 0; i < NB_BEEPS; i++)
    {
        ledcWriteTone(BoardState::LEDC_CHANNEL, 330UL);
        delay(BoardState::BEEP_DURATION_MS);
        ledcWrite(BoardState::LEDC_CHANNEL, 0UL);
        delay(BoardState::BEEP_DURATION_MS);
    }
}