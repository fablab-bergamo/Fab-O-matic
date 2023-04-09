#include <string>
#include <cstdint>

#include "BoardState.h"
#include "LCDWrapper.h"
#include "RFIDWrapper.h"
#include "pins.h"
#include "secrets.h"
#include <SPI.h>

namespace Board
{
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

    void BoardState::changeStatus(BoardStatus new_state)
    {
        this->state = new_state;
        this->update();
    }

    void BoardState::update()
    {
        Board::lcd.update(this->state, this->getMember());
    }

    bool BoardState::authorize(byte uid[10])
    {
        member.setUidFromArray(uid);
        if (this->getServer().isAuthorized(member))
        {
            this->member.setUidFromArray(uid);
            Board::machine.login(this->getMember());
            this->changeStatus(BoardStatus::LOGGED_IN);
            return true;
        }

        this->changeStatus(BoardStatus::LOGIN_DENIED);
        return false;
    }

    void BoardState::logout()
    {
        this->getMachine().logout();
        this->changeStatus(BoardStatus::LOGOUT);
    }
}