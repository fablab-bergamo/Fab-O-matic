#include <string>
#include <cstdint>

#include "BoardState.h"
#include "LCDWrapper.h"
#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "MFRC522Debug.h"
#include "pins.h"
#include "secrets.h"

BoardState::BoardState() {
  MFRC522DriverPinSimple ss_pin(pins::mfrc522::ss_pin); // Configurable, see typical pin layout above.
  MFRC522DriverSPI driver{ss_pin}; // Create SPI driver.
  // MFRC522DriverI2C driver{}; // Create I2C driver.
  MFRC522 mfrc522{driver}; // Create MFRC522 instance.
  LCDConfig config_lcd(pins::lcd::rs_pin, pins::lcd::en_pin, pins::lcd::d0_pin, pins::lcd::d1_pin, pins::lcd::d2_pin, pins::lcd::d3_pin);
  LCDWrapperType LCD(config_lcd);
  FabServer server(secrets::machine_data::whitelist, secrets::wifi::ssid, secrets::wifi::password);
  MachineConfig config1(secrets::machine_data::machine_id, MachineType::PRINTER3D, pins::relay::ch1_pin, false);
  Machine machine(config1);
  FabMember current_user = FabMember();

  this->rfid = &mfrc522;
  this->lcd = &LCD;
  this->machine = &machine;
  this->member = &current_user;
  this->server = &server;
}

void BoardState::init()
{
    this->getRfid().PCD_Init(); // Init MFRC522 board.
    this->getLCD().begin();
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

MFRC522 BoardState::getRfid()
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