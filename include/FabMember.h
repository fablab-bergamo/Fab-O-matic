#ifndef MEMBER_CARD_H
#define MEMBER_CARD_H

#include <cstdint>
#include <string>

namespace card
{
  typedef uint32_t uid_t;
}

class FabMember
{
private:
  card::uid_t member_uid = 0;
  std::string holder_name;

public:
  FabMember();
  ~FabMember() = default;

  void setUidFromArray(const uint8_t *uid); // tested
  void setUid(card::uid_t uid);             // tested
  card::uid_t getUid();                     // tested
  void setName(std::string name);           // tested
  std::string getName();                    // tested

  // define the copy constructor for FabMember
  FabMember &operator=(const FabMember &member);

  // Rule of 5 https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five
  // copy constructor
  FabMember(const FabMember &) = default;
  // move constructor
  FabMember(FabMember &&) = default;
  // move assignment
  FabMember &operator=(FabMember &&) = default;
};

#endif