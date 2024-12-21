#ifndef OTA_HPP
#define OTA_HPP

#include "ArduinoOTA.h"
#include <WiFiManager.h>

#include "secrets.hpp"

// For ArduinoOTA
const char *const ssid = fabomatic::secrets::credentials::ssid.data();
const char *const password = fabomatic::secrets::credentials::password.data();

namespace fabomatic
{
  enum struct ForceReset
  {
    True,
    False
  };
  enum struct DisablePortal { 
    True,
    False 
  };

  /// @brief Open the ArduinoOTA configuration portal
  /// @param force_reset true to discard saved settings and restore compile-time settings
  /// @param disable_portal true to skip portal opening (used at boot time)
  auto openConfigPortal(ForceReset force_reset, DisablePortal disable_portal) -> void;

  /// @brief Initialization of the Arduino OTA library with settings for fabomatic
  auto setupOTA() -> void;

} // namespace fabomatic
#endif // #ifndef OTA_HPP