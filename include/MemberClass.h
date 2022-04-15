#include <cstdint>
#include <string>
#ifndef MEMBER_CARD_H
#define MEMBER_CARD_H


namespace card{
  typedef uint32_t uid_t;
}

class MemberClass {
  private:
    card::uid_t member_uid;
    std::string holder_name; 
  public:
    MemberClass(/* args */);
    ~MemberClass();
    void setUidFromArray(const uint8_t* uid); // tested
    void setUid(card::uid_t uid); // tested
    card::uid_t getUid(); // tested
    void setName(std::string name); // tested
    std::string getName(); // tested

  // define the operator = for MemberCLass 
    MemberClass& operator = (const MemberClass &member);
};


#endif