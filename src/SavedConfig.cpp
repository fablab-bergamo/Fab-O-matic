#include <optional>
#include <algorithm>

#include <ArduinoJson.h>
#include <EEPROM.h>

#include "Logging.hpp"
#include "SavedConfig.hpp"
#include "conf.hpp"
#include "secrets.hpp"

namespace fablabbg
{
  // Define the static variable
  std::string SavedConfig::json_buffer;

  auto SavedConfig::setMachineID(MachineID id) -> void
  {
    machine_id = std::to_string(id);
  }
  auto SavedConfig::getMachineID() const -> MachineID
  {
    return MachineID(std::atoi(machine_id.c_str()));
  }

  auto SavedConfig::DefaultConfig() -> SavedConfig
  {
    SavedConfig config;

    // Wifi
    config.ssid = secrets::credentials::ssid;
    config.password = secrets::credentials::password;

    // MQTT
    config.mqtt_server = conf::default_config::mqtt_server;
    config.mqtt_user = secrets::credentials::mqtt_user;
    config.mqtt_password = secrets::credentials::mqtt_password;

    // MQTT Switch
    config.mqtt_switch_topic = conf::default_config::mqtt_switch_topic;

    // Machine
    config.setMachineID(conf::default_config::machine_id);
    config.magic_number = MAGIC_NUMBER;
    config.disablePortal = false;

    return config;
  }

  auto SavedConfig::LoadFromEEPROM() -> std::optional<SavedConfig>
  {
    SavedConfig::json_buffer.resize(JSON_DOC_SIZE);

    if (!EEPROM.begin(SavedConfig::json_buffer.size()))
    {
      ESP_LOGE(TAG, "SavedConfig::LoadFromEEPROM() : EEPROM.begin failed");
      return std::nullopt;
    }

    for (auto i = 0; i < SavedConfig::json_buffer.size(); i++)
    {
      SavedConfig::json_buffer[i] = EEPROM.readChar(i);
    }

    auto reply = fromJsonDocument(SavedConfig::json_buffer);
    if (!reply.has_value())
    {
      ESP_LOGW(TAG, "Failed to load config from EEPROM");
      return std::nullopt;
    }
    if (reply.value().magic_number != MAGIC_NUMBER)
    {
      ESP_LOGW(TAG, "Found different settings version in EEPROM (%d vs. %d), ignoring.", reply.value().magic_number, MAGIC_NUMBER);
      return std::nullopt;
    }
    return reply.value();
  }

  auto SavedConfig::toJsonDocument() const -> JsonDocument
  {
    JsonDocument doc;
    magic_number = MAGIC_NUMBER;
    doc["disablePortal"] = disablePortal;
    doc["bootCount"] = bootCount;
    doc["ssid"] = ssid;
    doc["password"] = password;
    doc["mqtt_server"] = mqtt_server;
    doc["mqtt_user"] = mqtt_user;
    doc["mqtt_password"] = mqtt_password;
    doc["mqtt_switch_topic"] = mqtt_switch_topic;
    doc["machine_id"] = machine_id;
    doc["magic_number"] = magic_number;
    auto cache = doc.createNestedArray("cachedRfid");
    for (const auto &entry : cachedRfid)
    {
      if (entry.uid != 0) // Skip empty entries
      {
        auto obj = cache.createNestedObject();
        obj["uid"] = entry.uid;
        obj["level"] = static_cast<uint8_t>(entry.level);
      }
    }
    return doc;
  }

  auto SavedConfig::fromJsonDocument(const std::string &json_text) -> std::optional<SavedConfig>
  {
    SavedConfig config;
    JsonDocument doc;

    auto result = deserializeJson(doc, json_text);
    if (result != DeserializationError::Ok)
    {
      ESP_LOGE(TAG, "fromJsonDocument() : deserializeJson failed with code %s", result.c_str());
      ESP_LOGE(TAG, "fromJsonDocument() : %s", json_text.c_str());
      return std::nullopt;
    }

    // Now map the JSON to the struct
    config.disablePortal = doc["disablePortal"];
    config.bootCount = doc["bootCount"];
    config.ssid = doc["ssid"].as<std::string>();
    config.password = doc["password"].as<std::string>();
    config.mqtt_server = doc["mqtt_server"].as<std::string>();
    config.mqtt_user = doc["mqtt_user"].as<std::string>();
    config.mqtt_password = doc["mqtt_password"].as<std::string>();
    config.mqtt_switch_topic = doc["mqtt_switch_topic"].as<std::string>();
    config.machine_id = doc["machine_id"].as<std::string>();
    config.magic_number = doc["magic_number"];

    auto idx = 0;
    for (const auto &elem : doc["cachedRfid"].as<JsonArray>())
    {
      auto uid = elem["uid"];
      auto level = static_cast<FabUser::UserLevel>(elem["level"].as<uint8_t>());
      config.cachedRfid.at(idx++) = {uid, level};
    }

    while (idx < conf::rfid_tags::CACHE_LEN)
    {
      config.cachedRfid.at(idx) = {0, FabUser::UserLevel::Unknown};
      idx++;
    }

    ESP_LOGD(TAG, "fromJsonDocument() : data deserialized successfully");

    return config;
  }

  auto SavedConfig::SaveToEEPROM() const -> bool
  {
    json_buffer.resize(JSON_DOC_SIZE);
    std::fill(SavedConfig::json_buffer.begin(), SavedConfig::json_buffer.begin() + SavedConfig::json_buffer.size(), '\0');

    if (!EEPROM.begin(JSON_DOC_SIZE))
    {
      ESP_LOGE(TAG, "SavedConfig::SaveToEEPROM() : EEPROM.begin failed");
      return false;
    }

    const auto &doc = toJsonDocument();
    serializeJson(doc, SavedConfig::json_buffer);

    for (auto i = 0; i < SavedConfig::json_buffer.size(); i++)
    {
      EEPROM.writeChar(i, SavedConfig::json_buffer[i]);
    }

    auto result = EEPROM.commit();

    if (result)
    {
      ESP_LOGD(TAG, "EEPROM commit success");
      return result;
    }

    ESP_LOGE(TAG, "EEPROM commit failure");
    return false;
  }

  auto SavedConfig::toString() const -> const std::string
  {
    const auto &doc = toJsonDocument();
    std::string result;
    serializeJsonPretty(doc, result);
    return result;
  }

  auto SavedConfig::IncrementBootCount() -> size_t
  {
    SavedConfig config = LoadFromEEPROM().value_or(DefaultConfig());
    config.bootCount++;
    if (!config.SaveToEEPROM())
    {
      ESP_LOGE(TAG, "Failed to save boot count to EEPROM");
    }
    return config.bootCount;
  }
} // namespace fablabbg
