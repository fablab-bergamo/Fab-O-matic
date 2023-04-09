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
  FabMember(const uint8_t *uid);
  ~FabMember() = default;

  void setUidFromArray(const uint8_t *uid);
  void setUid(card::uid_t uid);
  card::uid_t getUid() const;
  void setName(std::string name);
  std::string getName() const;

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