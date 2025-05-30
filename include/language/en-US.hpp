#ifndef _LANGUAGE_EN_US_HPP
#define _LANGUAGE_EN_US_HPP

namespace fabomatic::strings::en_US
{
  inline constexpr auto S_LANG_ID = "en-US";

  inline constexpr auto S_BUSY = "Busy";
  inline constexpr auto S_CANCELLED = "* CANCELLED *";
  inline constexpr auto S_CONFIRMED = "* CONFIRMED *";
  inline constexpr auto S_LONGTAP_PROMPT = "Record?";

  inline constexpr auto S_MACHINE_BLOCKED = "> BLOCKED <";
  inline constexpr auto S_MACHINE_MAINTENANCE = "Maintenance";
  inline constexpr auto S_CARD_PROMPT = "Pass your card";
  inline constexpr auto S_USED_BY = "In use by";

  inline constexpr auto S_START_USE = "Use started";
  inline constexpr auto S_LOGIN_DENIED = "Unknown card";
  inline constexpr auto S_GOODBYE = "Goodbye";

  inline constexpr auto S_CONNECTING_MQTT_1 = "Connecting";
  inline constexpr auto S_CONNECTING_MQTT_2 = "to MQTT server";

  inline constexpr auto S_CONNECTED = "Connected";
  inline constexpr auto S_HELLO = "Hello";

  inline constexpr auto S_WORKING = "Working...";
  inline constexpr auto S_OFFLINE_MODE = "OFFLINE MODE";

  inline constexpr auto S_BLOCKED_ADMIN_1 = "Blocked by";
  inline constexpr auto S_BLOCKED_ADMIN_2 = "admins";

  inline constexpr auto S_VERIFYING_1 = "Verifying";
  inline constexpr auto S_VERIFYING_2 = "card...";

  inline constexpr auto S_BLOCKED_MAINTENANCE_1 = "Blocked for";
  inline constexpr auto S_BLOCKED_MAINTENANCE_2 = "maintenance";

  inline constexpr auto S_PROMPT_MAINTENANCE_1 = "Maintenance";
  inline constexpr auto S_PROMPT_MAINTENANCE_2 = "record ?";

  inline constexpr auto S_MAINTENANCE_REGISTERED_1 = "Maintenance";
  inline constexpr auto S_MAINTENANCE_REGISTERED_2 = "recorded";

  inline constexpr auto S_GENERIC_ERROR = "Error";
  inline constexpr auto S_HW_ERROR = "HW error";
  inline constexpr auto S_PORTAL_ERROR = "Portal error";
  inline constexpr auto S_OTA_ERROR = "OTA error";

  inline constexpr auto S_PORTAL_SUCCESS = "WiFi conf. OK";
  inline constexpr auto S_OPEN_PORTAL = "Open portal";

  inline constexpr auto S_BOOTING = "Booting...";

  inline constexpr auto S_SHUTTING_DOWN = "Shutting down!";

  inline constexpr auto UPDATE_OTA_1 = "Upgrading";
  inline constexpr auto UPDATE_OTA_2 = "OTA...";

  inline constexpr auto FACTORY_RESET_DONE_1 = "Factory reset";
  inline constexpr auto FACTORY_RESET_DONE_2 = "Wait reboot";

  inline constexpr auto PORTAL_TITLE = "FAB-O-MATIC";
  inline constexpr auto PORTAL_MACHINE_ID_PROMPT = "Machine ID";
  inline constexpr auto PORTAL_SHELLY_MQTT_PROMPT = "Shelly MQTT topic (can be empty)";
  inline constexpr auto PORTAL_MQTT_BROKER_PROMPT = "MQTT broker (IP or hostname)";

} // namespace fabomatic::strings::en_US

#endif