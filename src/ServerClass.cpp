#include "ServerClass.h"
#include <string> 
#include <cstdint>

ServerClass::ServerClass(const std::array<card::uid_t, 10> whitelist, const std::string ssid, const std::string password)
: _whitelist(whitelist), _ssid(ssid), _password(password)
{
  online = false;
}


ServerClass::~ServerClass() {
}

bool ServerClass::isAuthorized(MemberClass & member_card) {
  if(this->isOnline()){
    return this->_serverQuery(member_card);
  }
  else{
    bool answer = this->_isWhiteListed(member_card);
    member_card.setName("MEMBER");
    return this->_isWhiteListed(member_card);
  }
}

bool ServerClass::isOnline() {
  return online;
}

void ServerClass::setOnline(bool online) {
  this->online = online;
}

bool ServerClass::_isWhiteListed(MemberClass member_card) {
  for(int i = 0; i < this->_whitelist.size(); i++){
    if(this->_whitelist[i] == member_card.getUid()){
      return true;
    }
  }
  return false;
}

bool ServerClass::_serverQuery(MemberClass member_card){
  if(member_card.getUid() == 0x11223344){
    return true;
  }
  else return false;
}

void ServerClass::connect(){
    for(auto i = 0; i < 3; i++) {
    this->WiFiConnection.begin(this->_ssid.c_str(), this->_password.c_str());
    if(this->WiFiConnection.status() == WL_CONNECTED) {

      break;
    }
    delay(1000);
  }
}