#ifndef SAVED_CONFIG_H_
#define SAVED_CONFIG_H_

#include <string>
#include <cstdint>
#include <optional>

#include "MachineConfig.hpp"
#include <EEPROM.h>

namespace fablabbg
{

  struct SavedConfig
  {
    static constexpr auto FIELD_LENGTH = 40;
    static constexpr auto INT_LENGTH = 5;      // Must save as string for WiFiManager
    static constexpr auto MAGIC_NUMBER = 0x44; // Increment when changing the struct

    // Wifi
    char ssid[FIELD_LENGTH]{0};
    char password[FIELD_LENGTH]{0};

    // MQTT
    char mqtt_server[FIELD_LENGTH]{0};
    char mqtt_user[FIELD_LENGTH]{0};
    char mqtt_password[FIELD_LENGTH]{0};

    // MQTT Switch
    char machine_topic[FIELD_LENGTH]{0};

    char machine_id[INT_LENGTH]{0};

    // Magic number to check if the EEPROM is initialized
    mutable uint8_t magic_number;

    bool SaveToEEPROM() const;
    std::string toString() const;

    static std::optional<SavedConfig> LoadFromEEPROM();
    static SavedConfig DefaultConfig();
  };

} // namespace fablabbg
#endif // SAVED_CONFIG_H_