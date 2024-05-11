#ifndef _LANGUAGE_IT_IT_HPP
#define _LANGUAGE_IT_IT_HPP

namespace fabomatic::strings::it_IT
{
  static constexpr auto S_BUSY = "Occupata";
  static constexpr auto S_CANCELLED = "* ANNULLATO *";
  static constexpr auto S_CONFIRMED = "* CONFERMATO *";
  static constexpr auto S_LONGTAP_PROMPT = "Registrare?";

  static constexpr auto S_MACHINE_BLOCKED = "> BLOCCATA <";
  static constexpr auto S_MACHINE_MAINTENANCE = "Manutenzione";
  static constexpr auto S_CARD_PROMPT = "Avvicina carta";
  static constexpr auto S_USED_BY = "In uso da";

  static constexpr auto S_START_USE = "Inizio uso";
  static constexpr auto S_LOGIN_DENIED = "Carta ignota";
  static constexpr auto S_GOODBYE = "Arrivederci";

  static constexpr auto S_CONNECTING_MQTT_1 = "Connessione";
  static constexpr auto S_CONNECTING_MQTT_2 = "al server MQTT";

  static constexpr auto S_CONNECTED = "MQTT connesso";
  static constexpr auto S_HELLO = "Ciao";

  static constexpr auto S_WORKING = "Elaborazione...";
  static constexpr auto S_OFFLINE_MODE = "OFFLINE MODE";

  static constexpr auto S_BLOCKED_ADMIN_1 = "Blocco";
  static constexpr auto S_BLOCKED_ADMIN_2 = "amministrativo";

  static constexpr auto S_VERIFYING_1 = "Verifica";
  static constexpr auto S_VERIFYING_2 = "in corso...";

  static constexpr auto S_BLOCKED_MAINTENANCE_1 = "Blocco per";
  static constexpr auto S_BLOCKED_MAINTENANCE_2 = "manutenzione";

  static constexpr auto S_PROMPT_MAINTENANCE_1 = "Manutenzione?";
  static constexpr auto S_PROMPT_MAINTENANCE_2 = "Registra";

  static constexpr auto S_MAINTENANCE_REGISTERED_1 = "Manutenzione";
  static constexpr auto S_MAINTENANCE_REGISTERED_2 = "registrata";

  static constexpr auto S_GENERIC_ERROR = "Errore";
  static constexpr auto S_HW_ERROR = "Errore HW";
  static constexpr auto S_PORTAL_ERROR = "Errore portale";
  static constexpr auto S_OTA_ERROR = "Errore OTA";
  static constexpr auto S_STATUS_ERROR_1 = "Unhandled status";
  static constexpr auto S_STATUS_ERROR_2 = "value";

  static constexpr auto S_PORTAL_SUCCESS = "Conf. WiFi OK";
  static constexpr auto S_OPEN_PORTAL = "Apri portale";

  static constexpr auto S_BOOTING = "Avvio...";

  static constexpr auto S_SHUTTING_DOWN = "In spegnimento!";

  static constexpr auto UPDATE_OTA_1 = "Aggiornamento";
  static constexpr auto UPDATE_OTA_2 = "OTA...";

  static constexpr auto FACTORY_RESET_DONE_1 = "Param. reset";
  static constexpr auto FACTORY_RESET_DONE_2 = "Attesa reboot";

  static constexpr auto PORTAL_TITLE = "FAB-O-MATIC";
  static constexpr auto PORTAL_MACHINE_ID_PROMPT = "ID della macchina";
  static constexpr auto PORTAL_SHELLY_MQTT_PROMPT = "Topic MQTT per Shelly (facoltativo)";
  static constexpr auto PORTAL_MQTT_BROKER_PROMPT = "Indirizzo o nome MQTT broker";
} // namespace fabomatic::strings::it_IT

#endif