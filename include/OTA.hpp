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

  /// @brief Open the ArduinoOTA configuration portal
  /// @param force_reset true to discard saved settings and restore compile-time settings
  /// @param disable_portal true to skip portal opening (used at boot time)
  void openConfigPortal(bool force_reset, bool disable_portal);
  
  void setupOTA();

} // namespace fabomatic
#endif // #ifndef OTA_HPP