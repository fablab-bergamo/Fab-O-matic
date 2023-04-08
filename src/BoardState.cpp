#include <string>
#include <cstdint>

#include "BoardState.h"
#include "LCDWrapper.h"
#include "RFIDWrapper.h"
#include "pins.h"
#include "secrets.h"
#include <SPI.h>

BoardState::BoardState() {
  RFIDWrapper rfid(pins::mfrc522::cs_pin);
  LCDConfig config_lcd(pins::lcd::rs_pin, pins::lcd::en_pin, pins::lcd::d0_pin, pins::lcd::d1_pin, pins::lcd::d2_pin, pins::lcd::d3_pin);
  LCDWrapperType LCD(config_lcd);
  FabServer server(secrets::machine_data::whitelist, secrets::wifi::ssid, secrets::wifi::password);
  MachineConfig config1(secrets::machine_data::machine_id, MachineType::PRINTER3D, pins::relay::ch1_pin, false);
  Machine machine(config1);
  FabMember current_user = FabMember();

  this->rfid = &rfid;
  this->lcd = &LCD;
  this->machine = &machine;
  this->member = &current_user;
  this->server = &server;
}

void BoardState::init()
{
    Serial.println("Initializing LCD...");
    this->getLCD().begin();
    delay(100);
    Serial.println("Initializing RFID...");
    this->getRfid().init(); // Init MFRC522 board.
    delay(100);
    Serial.print("Board init complete");
}

Machine BoardState::getMachine()
{
    return *this->machine;
}

FabMember BoardState::getMember()
{
    return *this->member;
}

FabServer BoardState::getServer()
{
    return *this->server;
}

LCDWrapperType BoardState::getLCD()
{
    return *this->lcd;
}

RFIDWrapper BoardState::getRfid()
{
    return *this->rfid;
}

void BoardState::changeStatus(BoardStatus new_state) {
    this->state = new_state;
    this->update();
}

void BoardState::update()
{
    this->getLCD().update(this->state, this->getServer(), this->getMember(), this->getMachine());
}

bool BoardState::authorize(byte uid[10])
{
    FabMember member;
    member.setUidFromArray(uid);
    if (this->getServer().isAuthorized(member)) {
      this->getMember().setUidFromArray(uid);
      this->getMachine().login(this->getMember());
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

BoardState &BoardState::operator=(const BoardState &member) {
  if (this == &member)
    return *this;

  this->lcd = member.lcd;
  this->rfid = member.rfid;
  this->state = member.state;
  this->machine = member.machine;
  this->server = member.server;
  return *this;
}