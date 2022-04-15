#include <cstdint> 
#include <string>
#include "MemberClass.h" 

// from https://stackoverflow.com/questions/58951095/c-c-convert-an-array-of-8-bytes-into-a-64-bit-integer
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!     this should be handled natively in C++ or by the compiler     !!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 
#define to32l(arr) (((uint32_t)(((uint8_t *)(arr))[3]) <<  0)+\
                    ((uint32_t)(((uint8_t *)(arr))[2]) <<  8)+\
                    ((uint32_t)(((uint8_t *)(arr))[1]) << 16)+\
                    ((uint32_t)(((uint8_t *)(arr))[0]) << 24))

// take that, little endian!
#define to32b(arr) *(card::uid_t *)arr

MemberClass::MemberClass(/* args */)
{
  // variable init
  MemberClass::member_uid = 0x0;
  MemberClass::holder_name = ""; 
}

MemberClass::~MemberClass()
{
  // idk dude, what shoud i do here?
}

MemberClass& MemberClass::operator= (const MemberClass& member) { 
      this->member_uid = member.member_uid;
      this->holder_name = member.holder_name;
      return *this;
}

void MemberClass::setUidFromArray(const uint8_t* uid)
{
  MemberClass::member_uid = to32l(uid);
}
void MemberClass::setUid(card::uid_t uid)
{
  MemberClass::member_uid = uid;
}

card::uid_t MemberClass::getUid()
{
  return MemberClass::member_uid;
}

void MemberClass::setName(std::string name)
{
  MemberClass::holder_name = name;
}


std::string MemberClass::getName()
{
  return MemberClass::holder_name;
}
