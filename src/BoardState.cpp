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

namespace Board
{
    // Only main.cpp instanciates the variables through Board.h file
    extern RFIDWrapper rfid;
    extern LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd;
    extern FabServer server;
    extern Machine machine;
    extern AuthProvider auth;
}

void BoardState::init()
{
    Serial.println("Initializing LCD...");
    Board::lcd.begin();
    delay(100);
    Serial.println("Initializing RFID...");
    Board::rfid.init();
    delay(100);
    Serial.println("Board init complete");
}

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

void BoardState::update()
{
    char buffer[conf::lcd::COLS];
    std::string user_name, machine_name, uid_str;

    Board::lcd.showConnection(true);
    Board::lcd.showPower(true);

    user_name = Board::machine.getActiveUser().holder_name;
    machine_name = Board::machine.getMachineName();
    uid_str = card::uid_str(this->member.member_uid);

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
        Board::lcd.setRow(0, "BLOCCATA DA");
        Board::lcd.setRow(1, "FABLAB");
        break;
    case Status::VERIFYING:
        Board::lcd.setRow(0, "VERIFICA IN");
        Board::lcd.setRow(1, "CORSO");
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

bool BoardState::authorize(card::uid_t uid)
{
    FabUser member;
    this->changeStatus(Status::VERIFYING);
    if (Board::auth.tryLogin(uid, member))
    {
        if (Board::machine.allowed)
        {
            Board::machine.login(member);
            auto result = Board::server.startUse(Board::machine.getActiveUser().member_uid, Board::machine.getMachineId());
            Serial.printf("Result startUse: %d\n", result.request_ok);
            this->member = member;
            this->changeStatus(Status::LOGGED_IN);
            return true;
        }
        this->changeStatus(Status::NOT_ALLOWED);
        return false;
    }
    else
    {
        Serial.println("Failed login");
    }
    this->changeStatus(Status::LOGIN_DENIED);
    return false;
}

void BoardState::logout()
{
    auto result = Board::server.finishUse(Board::machine.getActiveUser().member_uid, Board::machine.getMachineId(), Board::machine.getUsageTime());
    Serial.printf("Result finishUse: %d\n", result.request_ok);
    Board::machine.logout();
    this->member = FabUser();
    this->changeStatus(Status::LOGOUT);
    delay(1000);
}

BoardState::Status BoardState::getStatus()
{
    return this->status;
}

FabUser BoardState::getMember()
{
    return this->member;
}