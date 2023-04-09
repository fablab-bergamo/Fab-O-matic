#include <string>
#include <cstdint>

#include "BoardState.h"
#include "conf.h"
#include "RFIDWrapper.h"
#include "LCDWrapper.h"
#include "FabServer.h"
#include "Machine.h"

namespace Board
{
    // Only main.cpp instanciates the variables through Board.h file
    extern RFIDWrapper rfid;
    extern LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> lcd;
    extern FabServer server;
    extern Machine machine;
}

BoardState::BoardState()
{
    FabMember current_user = FabMember();
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
    std::string user_name;
    user_name = Board::machine.getActiveUser().getName();

    switch (this->status)
    {
    case Status::CLEAR:
        Board::lcd.clear();
        break;
    case Status::FREE:
        Board::lcd.setRow(0, Board::server.isOnline() ? "Disponibile" : "OFFLINE");
        Board::lcd.setRow(1, "Avvicina carta");
        break;
    case Status::ALREADY_IN_USE:
        Board::lcd.setRow(0, "In uso da");
        Board::lcd.setRow(1, Board::machine.getActiveUser().getName());
        break;
    case Status::LOGGED_IN:
        Board::lcd.setRow(0, "Inizio uso");
        Board::lcd.setRow(1, this->member.getName());
        break;
    case Status::LOGIN_DENIED:
        Board::lcd.setRow(0, "Negato");
        Board::lcd.setRow(1, "Carta ignota");
        break;
    case Status::LOGOUT:
        Board::lcd.setRow(0, "Arrivederci");
        Board::lcd.setRow(1, this->member.getName());
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

bool BoardState::authorize(byte uid[10])
{
    member.setUidFromArray(uid);
    if (Board::server.isAuthorized(member))
    {
        this->member.setUidFromArray(uid);
        Board::machine.login(this->getMember());
        this->changeStatus(Status::LOGGED_IN);
        return true;
    }

    this->changeStatus(Status::LOGIN_DENIED);
    return false;
}

void BoardState::logout()
{
    Board::machine.logout();
    this->changeStatus(Status::LOGOUT);
}

BoardState::Status BoardState::getStatus()
{
    return this->status;
}

FabMember BoardState::getMember()
{
    return this->member;
}