#ifndef _BOARDSTATE_H_
#define _BOARDSTATE_H_

#include "FabMember.h"
#include "FabServer.h"
#include "Machine.h"
#include "LCDWrapper.h"
#include "BoardStatus.h"
#include "conf.h"
#include "RFIDWrapper.h"
#include "pins.h"
#include "secrets.h"

typedef LCDWrapper<conf::lcd::COLS, conf::lcd::ROWS> LCDWrapperType;

namespace Board
{
    // Variables
    static RFIDWrapper rfid(pins::mfrc522::cs_pin);
    static LCDWrapperType::Config config_lcd(pins::lcd::rs_pin, pins::lcd::en_pin, pins::lcd::d0_pin, pins::lcd::d1_pin, pins::lcd::d2_pin, pins::lcd::d3_pin);
    static LCDWrapperType lcd(config_lcd);
    static FabServer server(secrets::machine_data::whitelist, secrets::wifi::ssid, secrets::wifi::password);
    static Machine::Config config1(secrets::machine_data::machine_id, Machine::MachineType::PRINTER3D, pins::relay::ch1_pin, false);
    static Machine machine(config1);

    class BoardState 
    {
    private:
        BoardStatus state;
        FabMember member;

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

        // copy reference
        BoardState &operator=(const BoardState &member) = delete;
        // copy constructor
        BoardState(const BoardState &) = delete;
        // move constructor
        BoardState(BoardState &&) = default;
        // move assignment
        BoardState &operator=(BoardState &&) = default;
    };
}
#endif