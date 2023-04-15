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
    Status getStatus() const;
    FabUser getUser();

    bool init();
    void changeStatus(Status newStatus);
    void update();
    bool authorize(card::uid_t uid);
    void logout();
    void beep_ok();
    void beep_failed();

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
    static constexpr unsigned short LEDC_CHANNEL = 0U;          /* Esp32 pwm channel for beep generation */
    static constexpr unsigned short BEEP_DURATION_MS = 250UL;   /* Beep duration in milliseconds */
};
#endif