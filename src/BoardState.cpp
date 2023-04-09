#include <string>
#include <cstdint>

#include "BoardState.h"
#include "conf.h"
#include "RFIDWrapper.h"
#include "LCDWrapper.h"
#include "FabServer.h"
#include "Machine.h"
#include "AuthProvider.h"

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
        char buffer[32]={0};
        sprintf(buffer, "** Changing board state to %d", static_cast<typename std::underlying_type<Status>::type>(new_state));
        Serial.println(buffer);
    }

    this->status = new_state;
    this->update();
}

void BoardState::update()
{
    char buffer[conf::lcd::COLS];
    Board::lcd.showConnection(true);
    std::string user_name, machine_name, uid_str;
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
        Board::lcd.setRow(1, "Avvicina carta");
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
        Board::lcd.setRow(0, "Connessione in");
        Board::lcd.setRow(1, "corso al server...");
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
    }
    Board::lcd.update_chars(Board::server.isOnline());
}

bool BoardState::authorize(card::uid_t uid)
{
    FabUser member;
    if (Board::auth.tryLogin(uid, member)) {
        Board::machine.login(member);
        this->member = member;
        this->changeStatus(Status::LOGGED_IN);
        return true;
    }
    this->changeStatus(Status::LOGIN_DENIED);
    return false;
}

void BoardState::logout()
{
    Board::machine.logout();
    this->member = FabUser();
    this->changeStatus(Status::LOGOUT);
}

BoardState::Status BoardState::getStatus()
{
    return this->status;
}

FabUser BoardState::getMember()
{
    return this->member;
}