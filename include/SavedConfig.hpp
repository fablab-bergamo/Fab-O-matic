#ifndef SAVEDCONFIG_HPP_
#define SAVEDCONFIG_HPP_

#include <cstdint>
#include <optional>
#include <string>
#include <mutex>

#include <EEPROM.h>
#include <ArduinoJson.h>

#include "MachineConfig.hpp"
#include "conf.hpp"
#include "CachedCards.hpp"

namespace fabomatic
{
  class SavedConfig
  {
  private:
    static constexpr auto JSON_DOC_SIZE = 1024;
    static_assert(JSON_DOC_SIZE > (conf::common::STR_MAX_LENGTH * 7 + sizeof(bool) + sizeof(size_t) + sizeof(CachedCards)) * 2, "JSON_DOC_SIZE must be larger than SavedConfig size in JSON");
    static std::string json_buffer;
    static std::mutex buffer_mutex;

    /// @brief Serialize the current configuration into a JsonDocument
    /// @return JsonDocument
    [[nodiscard]] auto toJsonDocument() const -> JsonDocument;

    /// @brief Deserialize a JsonDocument into a SavedConfig
    /// @param json_text json document as string to be deserialized
    /// @return std::nullopt if the document is invalid, or a valid SavedConfig
    [[nodiscard]] static auto fromJsonDocument(const std::string &json_text) -> std::optional<SavedConfig>;

  public:
    static constexpr auto MAGIC_NUMBER = 0x50; // Increment when changing the struct

    // Magic number to check if the EEPROM is initialized
    mutable uint8_t magic_number{0};

    /// @brief list of cached RFID cards
    CachedCards cachedRfid;

    /// @brief WiFi SSID
    std::string ssid{""};

    /// @brief WiFi password
    std::string password{""};

    /// @brief MQTT server
    std::string mqtt_server{""};

    /// @brief MQTT username
    std::string mqtt_user{""};

    /// @brief MQTT password
    std::string mqtt_password{""};

    /// @brief MQTT topic for mqtt switch (if available)
    std::string mqtt_switch_topic{""};

    /// @brief Machine ID connected to the board
    /// @details This is a string to allow for easier integration with WiFiManager parameter
    std::string machine_id{""};

    /// @brief number of boot cycles
    size_t bootCount{0};

    /// @brief if true, the FORCE_OPEN_PORTAL flag will be ignored
    bool disablePortal{false};

    /// @brief Allow compiler-time construction
    constexpr SavedConfig() = default;

    /// @brief Saves the configuration to EEPROM
    /// @return true if successful
    auto SaveToEEPROM() const -> bool;

    /// @brief Sets the machine ID (converting to string)
    auto setMachineID(MachineID id) -> void;

    /// @brief Gets the machine ID (converting from string)
    [[nodiscard]] auto getMachineID() const -> MachineID;

    /// @brief Returns a json-prettified fragment for logging
    [[nodiscard]] auto toString() const -> const std::string;

    /// @brief Loads the configuration from EEPROM if available and matching revision number
    /// @return std::nullopt if not valid, SavedConfig otherwise
    [[nodiscard]] static auto LoadFromEEPROM() -> std::optional<SavedConfig>;

    /// @brief Returns the default configuration built from conf.hpp and secrets.hpp
    [[nodiscard]] static auto DefaultConfig() -> SavedConfig;

    /// @brief Increments the boot count and saves it to EEPROM
    static auto IncrementBootCount() -> size_t;
  };

} // namespace fabomatic
#endif // SAVEDCONFIG_HPP_