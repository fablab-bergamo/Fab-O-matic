#include <cstdint>
#include <array>
#include <string>
#include "MachineClass.h"
#include "MemberClass.h"

namespace secrets{
namespace machine_data{
const std::string machine_name = "EXAMPLE_MACHINE";
constexpr MachineIDType machine_id = 0x12345678;
constexpr std::array<card::uid_t, 10> whitelist = {
  0xF6F07894,
  0xAABBCCDD,
  0xAABBCCDD,
  0xAABBCCDD,
  0xAABBCCDD,
  0xAABBCCDD
};
}

namespace wifi{
  const std::string ssid = "EXAMPLE_SSID";
  const std::string password = "EXAMPLE_PASSWORD";
}
}