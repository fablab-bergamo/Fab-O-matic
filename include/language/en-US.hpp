#ifndef _LANGUAGE_EN_US_HPP
#define _LANGUAGE_EN_US_HPP

namespace fablabbg::strings::en_US
{
  static constexpr auto S_BUSY = "Busy";
  static constexpr auto S_CANCELLED = "* CANCELLED *";
  static constexpr auto S_CONFIRMED = "* CONFIRMED *";
  static constexpr auto S_LONGTAP_PROMPT = "Record?";

  static constexpr auto S_MACHINE_BLOCKED = "> BLOCKED <";
  static constexpr auto S_MACHINE_MAINTENANCE = "Maintenance";
  static constexpr auto S_CARD_PROMPT = "Pass your card";
  static constexpr auto S_USED_BY = "In use by";

  static constexpr auto S_START_USE = "Use started";
  static constexpr auto S_LOGIN_DENIED = "Unknown card";
  static constexpr auto S_GOODBYE = "Goodbye";

  static constexpr auto S_CONNECTING_MQTT_1 = "Connecting";
  static constexpr auto S_CONNECTING_MQTT_2 = "to MQTT server";

  static constexpr auto S_CONNECTED = "Connected";
  static constexpr auto S_HELLO = "Hello";

  static constexpr auto S_WORKING = "Working...";
  static constexpr auto S_OFFLINE_MODE = "OFFLINE MODE";

  static constexpr auto S_BLOCKED_ADMIN_1 = "Blocked";
  static constexpr auto S_BLOCKED_ADMIN_2 = "by admin";

  static constexpr auto S_VERIFYING_1 = "Verifying";
  static constexpr auto S_VERIFYING_2 = "card...";

  static constexpr auto S_BLOCKED_MAINTENANCE_1 = "Blocked for";
  static constexpr auto S_BLOCKED_MAINTENANCE_2 = "maintenance";

  static constexpr auto S_PROMPT_MAINTENANCE_1 = "Maintenance?";
  static constexpr auto S_PROMPT_MAINTENANCE_2 = "Record";

  static constexpr auto S_MAINTENANCE_REGISTERED_1 = "Maintenance";
  static constexpr auto S_MAINTENANCE_REGISTERED_2 = "recorded";

  static constexpr auto S_GENERIC_ERROR = "Error";
  static constexpr auto S_HW_ERROR = "HW error";
  static constexpr auto S_PORTAL_ERROR = "Portal error";
  static constexpr auto S_OTA_ERROR = "OTA error";
  static constexpr auto S_STATUS_ERROR_1 = "Unhandled status";
  static constexpr auto S_STATUS_ERROR_2 = "value";

  static constexpr auto S_PORTAL_SUCCESS = "WiFi conf. OK";
  static constexpr auto S_OPEN_PORTAL = "Open portal";

  static constexpr auto S_BOOTING = "Booting...";

  static constexpr auto S_SHUTTING_DOWN = "Shutting down!";

  static constexpr auto UPDATE_OTA_1 = "Upgrading";
  static constexpr auto UPDATE_OTA_2 = "OTA...";

  static constexpr auto FACTORY_RESET_DONE_1 = "Factory reset";
  static constexpr auto FACTORY_RESET_DONE_2 = "Wait reboot";

}
#endif