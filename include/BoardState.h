#ifndef _BOARDSTATE_H_
#define _BOARDSTATE_H_

#include "FabMember.h"
#include "Arduino.h"

class BoardState
{
public:
    enum class Status
    {
        CLEAR,
        FREE,
        LOGGED_IN,
        LOGIN_DENIED,
        BUSY,
        LOGOUT,
        CONNECTING,
        CONNECTED,
        ALREADY_IN_USE,
        IN_USE,
        OFFLINE
    };

    BoardState();
    Status getStatus();
    FabMember getMember();

    void init();
    void changeStatus(Status newStatus);
    void update();
    bool authorize(byte uid[10]);
    void logout();

    // copy reference
    BoardState &operator=(const BoardState &member) = delete;
    // copy constructor
    BoardState(const BoardState &) = delete;
    // move constructor
    BoardState(BoardState &&) = default;
    // move assignment
    BoardState &operator=(BoardState &&) = default;

private:
    Status status;
    FabMember member;
};
#endif