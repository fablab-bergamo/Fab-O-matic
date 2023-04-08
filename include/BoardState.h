#ifndef _BOARDSTATE_H_
#define _BOARDSTATE_H_

#include "FabMember.h"
#include "FabServer.h"
#include "Machine.h"
#include "LCDWrapper.h"
#include "BoardStatus.h"
#include "conf.h"
#include "RFIDWrapper.h"

typedef LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> LCDWrapperType;

class BoardState 
{
private:
    Machine *machine;
    FabMember *member;
    FabServer *server;
    LCDWrapperType *lcd;
    BoardStatus state;
    RFIDWrapper *rfid;

public:
    BoardState();
    Machine getMachine();
    FabMember getMember();
    FabServer getServer();
    LCDWrapperType getLCD();
    RFIDWrapper getRfid();

    void init();
    void changeStatus(BoardStatus newStatus);
    void update();
    bool authorize(byte uid[10]);
    void logout();

    BoardState &operator=(const BoardState &member);
    // copy constructor
    BoardState(const BoardState &) = default;
    // move constructor
    BoardState(BoardState &&) = default;
    // move assignment
    BoardState &operator=(BoardState &&) = default;
};
#endif