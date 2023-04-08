#include <cstdint>
#include <string>
#include "FabMember.h"

// from https://stackoverflow.com/questions/58951095/c-c-convert-an-array-of-8-bytes-into-a-64-bit-integer
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!     this should be handled natively in C++ or by the compiler     !!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define to32l(arr) (((uint32_t)(((uint8_t *)(arr))[3]) << 0) +  \
                    ((uint32_t)(((uint8_t *)(arr))[2]) << 8) +  \
                    ((uint32_t)(((uint8_t *)(arr))[1]) << 16) + \
                    ((uint32_t)(((uint8_t *)(arr))[0]) << 24))

// take that, little endian!
#define to32b(arr) *(card::uid_t *)arr

FabMember::FabMember() : member_uid(0x0), holder_name("?") {}

FabMember::FabMember(const u_int8_t *uid) : holder_name("?")
{
  // variable init
  this->member_uid = to32l(uid);
}

FabMember &FabMember::operator=(const FabMember &member)
{
  if (this == &member)
    return *this;

  this->member_uid = member.member_uid;
  this->holder_name = member.holder_name;
  return *this;
}

void FabMember::setUidFromArray(const uint8_t *uid)
{
  FabMember::member_uid = to32l(uid);
}
void FabMember::setUid(card::uid_t uid)
{
  FabMember::member_uid = uid;
}

card::uid_t FabMember::getUid() const
{
  return FabMember::member_uid;
}

void FabMember::setName(std::string name)
{
  FabMember::holder_name = name;
}

std::string FabMember::getName()
{
  return FabMember::holder_name;
}
