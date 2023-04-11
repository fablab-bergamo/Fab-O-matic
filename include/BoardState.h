#ifndef _BOARDSTATE_H_
#define _BOARDSTATE_H_

#include "FabUser.h"
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
        OFFLINE,
        NOT_ALLOWED,
        VERIFYING,
        MAINTENANCE_NEEDED
    };

    BoardState() = default;
    Status getStatus();
    FabUser getMember();

    void init();
    void changeStatus(Status newStatus);
    void update();
    bool authorize(card::uid_t uid);
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
    FabUser member;
};
#endif