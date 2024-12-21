#ifndef _LANGUAGE_IT_IT_HPP
#define _LANGUAGE_IT_IT_HPP

namespace fabomatic::strings::it_IT
{
  inline constexpr auto S_LANG_ID = "it-IT";

  inline constexpr auto S_BUSY = "Occupata";
  inline constexpr auto S_CANCELLED = "* ANNULLATO *";
  inline constexpr auto S_CONFIRMED = "* CONFERMATO *";
  inline constexpr auto S_LONGTAP_PROMPT = "Registrare?";

  inline constexpr auto S_MACHINE_BLOCKED = "> BLOCCATA <";
  inline constexpr auto S_MACHINE_MAINTENANCE = "Manutenzione";
  inline constexpr auto S_CARD_PROMPT = "Avvicina carta";
  inline constexpr auto S_USED_BY = "In uso da";

  inline constexpr auto S_START_USE = "Inizio uso";
  inline constexpr auto S_LOGIN_DENIED = "Carta ignota";
  inline constexpr auto S_GOODBYE = "Arrivederci";

  inline constexpr auto S_CONNECTING_MQTT_1 = "Connessione";
  inline constexpr auto S_CONNECTING_MQTT_2 = "al server MQTT";

  inline constexpr auto S_CONNECTED = "MQTT connesso";
  inline constexpr auto S_HELLO = "Ciao";

  inline constexpr auto S_WORKING = "Elaborazione...";
  inline constexpr auto S_OFFLINE_MODE = "OFFLINE MODE";

  inline constexpr auto S_BLOCKED_ADMIN_1 = "Blocco";
  inline constexpr auto S_BLOCKED_ADMIN_2 = "amministrativo";

  inline constexpr auto S_VERIFYING_1 = "Verifica";
  inline constexpr auto S_VERIFYING_2 = "in corso...";

  inline constexpr auto S_BLOCKED_MAINTENANCE_1 = "Blocco per";
  inline constexpr auto S_BLOCKED_MAINTENANCE_2 = "manutenzione";

  inline constexpr auto S_PROMPT_MAINTENANCE_1 = "Manutenzione?";
  inline constexpr auto S_PROMPT_MAINTENANCE_2 = "Registra";

  inline constexpr auto S_MAINTENANCE_REGISTERED_1 = "Manutenzione";
  inline constexpr auto S_MAINTENANCE_REGISTERED_2 = "registrata";

  inline constexpr auto S_GENERIC_ERROR = "Errore";
  inline constexpr auto S_HW_ERROR = "Errore HW";
  inline constexpr auto S_PORTAL_ERROR = "Errore portale";
  inline constexpr auto S_OTA_ERROR = "Errore OTA";

  inline constexpr auto S_PORTAL_SUCCESS = "Conf. WiFi OK";
  inline constexpr auto S_OPEN_PORTAL = "Apri portale";

  inline constexpr auto S_BOOTING = "Avvio...";

  inline constexpr auto S_SHUTTING_DOWN = "In spegnimento!";

  inline constexpr auto UPDATE_OTA_1 = "Aggiornamento";
  inline constexpr auto UPDATE_OTA_2 = "OTA...";

  inline constexpr auto FACTORY_RESET_DONE_1 = "Param. reset";
  inline constexpr auto FACTORY_RESET_DONE_2 = "Attesa reboot";

  inline constexpr auto PORTAL_TITLE = "FAB-O-MATIC";
  inline constexpr auto PORTAL_MACHINE_ID_PROMPT = "ID della macchina";
  inline constexpr auto PORTAL_SHELLY_MQTT_PROMPT = "Topic MQTT per Shelly (facoltativo)";
  inline constexpr auto PORTAL_MQTT_BROKER_PROMPT = "Indirizzo o nome MQTT broker";
} // namespace fabomatic::strings::it_IT

#endif