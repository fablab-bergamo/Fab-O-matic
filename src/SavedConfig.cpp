#include <optional>

#include <ArduinoJson.h>
#include <EEPROM.h>

#include "SavedConfig.hpp"
#include "secrets.hpp"
#include "conf.hpp"

namespace fablabbg
{
  SavedConfig SavedConfig::DefaultConfig()
  {
    SavedConfig config;

    // Wifi
    strncpy(config.ssid, conf::default_config::ssid.data(), FIELD_LENGTH);
    strncpy(config.password, conf::default_config::password.data(), FIELD_LENGTH);

    // MQTT
    strncpy(config.mqtt_server, conf::default_config::mqtt_server.data(), FIELD_LENGTH);
    strncpy(config.mqtt_user, conf::default_config::mqtt_user.data(), FIELD_LENGTH);
    strncpy(config.mqtt_password, conf::default_config::mqtt_password.data(), FIELD_LENGTH);

    // MQTT Switch
    strncpy(config.machine_topic, conf::default_config::machine_topic.data(), FIELD_LENGTH);

    // Machine
    strncpy(config.machine_id, std::to_string(conf::default_config::machine_id.id).c_str(), INT_LENGTH);

    return config;
  }

  std::optional<SavedConfig> SavedConfig::LoadFromEEPROM()
  {
    SavedConfig config;

    if (!EEPROM.begin(sizeof(SavedConfig)))
    {
      if (conf::debug::ENABLE_LOGS)
      {
        Serial.println("SavedConfig::LoadFromEEPROM() : EEPROM.begin failed");
      }
      return std::nullopt;
    }

    EEPROM.get(0, config);

    if (config.magic_number != MAGIC_NUMBER)
    {
      return std::nullopt;
    }
    return config;
  }

  bool SavedConfig::SaveToEEPROM() const
  {
    if (!EEPROM.begin(sizeof(SavedConfig)))
    {
      if (conf::debug::ENABLE_LOGS)
      {
        Serial.println("SavedConfig::SaveToEEPROM() : EEPROM.begin failed");
      }
      return false;
    }

    this->magic_number = MAGIC_NUMBER;

    EEPROM.put(0, *this);
    auto result = EEPROM.commit();

    if (conf::debug::ENABLE_LOGS)
    {
      Serial.printf("EEPROM commit result: %d\r\n", result);
      Serial.println("EEPROM content:");
      Serial.println(this->toString().c_str());
    }
    return result;
  }

  std::string SavedConfig::toString() const
  {
    DynamicJsonDocument doc(1024);

    doc["ssid"] = this->ssid;
    doc["password"] = this->password;
    doc["mqtt_server"] = this->mqtt_server;
    doc["mqtt_user"] = this->mqtt_user;
    doc["mqtt_password"] = this->mqtt_password;
    doc["machine_topic"] = this->machine_topic;
    doc["machine_id"] = this->machine_id;

    std::string result;
    serializeJson(doc, result);
    return result;
  }
}
