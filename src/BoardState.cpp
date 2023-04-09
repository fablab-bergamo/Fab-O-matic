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
    Board::rfid.init(); // Init MFRC522 board.
    delay(100);
    Serial.print("Board init complete");
}

void BoardState::changeStatus(Status new_state)
{
    this->status = new_state;
    this->update();
}

void BoardState::update()
{
    char buffer[conf::lcd::COLS];

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
        Board::lcd.setRow(1, "Carta sconosciuta");
        break;
    case Status::LOGOUT:
        Board::lcd.setRow(0, "Arrivederci");
        Board::lcd.setRow(1, this->member.getName());
        break;
    case Status::CONNECTING:
        Board::lcd.setRow(0, "Connecting");
        break;
    case Status::CONNECTED:
        Board::lcd.setRow(0, "Connected");
        break;
    case Status::IN_USE:
        snprintf(buffer, sizeof(buffer), "Ciao %s", Board::machine.getActiveUser().getName());
        Board::lcd.setRow(0, buffer);
        Board::lcd.setRow(1, Board::lcd.convertSecondsToHHMMSS(Board::machine.getUsageTime()));
        break;
    case Status::BUSY:
        Board::lcd.setRow(0, "Busy");
        break;
    case Status::OFFLINE:
        Board::lcd.setRow(0, "OFFLINE MODE");
        break;
    }
    Board::lcd.update_chars();
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