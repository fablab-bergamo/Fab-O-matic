#ifndef BOARDSTATE_H_
#define BOARDSTATE_H_

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
        MAINTENANCE_NEEDED,
        MAINTENANCE_QUERY,
        MAINTENANCE_DONE,
        ERROR
    };

    BoardState() = default;
    Status getStatus() const;
    FabUser getUser();

    bool init();
    void changeStatus(Status newStatus);
    void update();
    bool authorize(const card::uid_t uid);
    void logout();
    void beep_ok() const;
    void beep_failed() const;

    // copy reference
    BoardState &operator=(const BoardState &member) = delete;
    // copy constructor
    BoardState(const BoardState &) = delete;
    // move constructor
    BoardState(BoardState &&) = default;
    // move assignment
    BoardState &operator=(BoardState &&) = default;

    u_int16_t no_card_cpt = 0;
    unsigned long last_server_poll = 1;
    bool ready_for_a_new_card = true;

private:
    Status status;
    FabUser member;
    static constexpr unsigned short LEDC_CHANNEL = 0U;        /* Esp32 pwm channel for beep generation */
    static constexpr unsigned short BEEP_DURATION_MS = 200UL; /* Beep duration in milliseconds */
    static constexpr unsigned int BEEP_HZ = 660U;
};
#endif // BOARDSTATE_H_
