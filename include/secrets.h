#ifndef _SECRETS_H_
#define _SECRETS_H_

#include <cstdint>
#include <array>
#include <string>
#include "Machine.h"
#include "conf.h"

namespace secrets
{
  namespace machine_data
  {
    const std::string machine_name = "EXAMPLE_MACHINE";
    static constexpr uint16_t machine_id = (uint16_t)12345678; /* Machine connected to the ESP32 */
    static constexpr std::array<card::uid_t, conf::whitelist::LEN> whitelist /* List of RFID tags whitelisted, regardless of connection */
    {
        0xF6F07894,
        0xAABBCCDD,
        0xAABBCCDD,
        0xAABBCCDD,
        0xAABBCCDD,
        0xAABBCCDD,
        0xAABBCCDD,
        0xAABBCCDD,
        0xAABBCCDD,
        0xAABBCCDD};
  }

  namespace wifi
  {
    const std::string ssid = "EXAMPLE_SSID"; /* Change with WIFI SSID name */
    const std::string password = "EXAMPLE_PASSWORD"; /* Change with WIFI SSID password */
  }
}
#endif