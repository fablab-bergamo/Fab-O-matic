#include <atomic>

#include "OTA.hpp"
#include "BoardLogic.hpp"
#include "language/lang.hpp"

namespace fabomatic
{
  using Status = BoardLogic::Status;

  std::atomic<bool> shouldSaveConfig{false};

  namespace Board
  {
    extern BoardLogic logic;
  } // namespace Board

  // callback notifying us of the need to save config
  void saveConfigCallback()
  {
    shouldSaveConfig.store(true);
  }

  // Called just before the webportal is started after failed WiFi connection
  void configModeCallback(WiFiManager *myWiFiManager)
  {
    ESP_LOGI(TAG, "Entering portal config mode");
    ESP_LOGD(TAG, "IP: %s", WiFi.softAPIP().toString().c_str());
    ESP_LOGD(TAG, "SSID: %s", myWiFiManager->getConfigPortalSSID().c_str());
    Board::logic.changeStatus(Status::PortalStarting);
  }

  auto getConfig(bool force_reset) -> SavedConfig
  {
    const auto &opt_settings = SavedConfig::LoadFromEEPROM();
    if (force_reset || !opt_settings)
    {
      return SavedConfig::DefaultConfig();
    }

    return opt_settings.value();
  }

  // Starts the WiFi and possibly open the config portal in a blocking manner
  /// @param force_reset if true, the portal will be reset to factory defaults
  /// @param disable_portal if true, the portal will be disabled (useful at boot-time)
  void openConfigPortal(bool force_reset, bool disable_portal)
  {
    WiFiManager wifiManager;
    auto config = getConfig(force_reset);

    // We are using config as a buffer for the WiFiManager parameters, make sure it can hold the content
    config.mqtt_server.resize(conf::common::STR_MAX_LENGTH);
    config.mqtt_switch_topic.resize(conf::common::STR_MAX_LENGTH);
    config.machine_id.resize(conf::common::INT_MAX_LENGTH);

    WiFiManagerParameter custom_mqtt_server("Broker", strings::PORTAL_MQTT_BROKER_PROMPT, config.mqtt_server.data(), conf::common::STR_MAX_LENGTH);
    WiFiManagerParameter custom_mqtt_topic("Topic", strings::PORTAL_SHELLY_MQTT_PROMPT, config.mqtt_switch_topic.data(), conf::common::STR_MAX_LENGTH);
    WiFiManagerParameter custom_machine_id("MachineID", strings::PORTAL_MACHINE_ID_PROMPT, config.machine_id.data(), conf::common::INT_MAX_LENGTH, "type='number' min='0' max='65535'");

    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_topic);
    wifiManager.addParameter(&custom_machine_id);

    wifiManager.setTimeout(std::chrono::duration_cast<std::chrono::seconds>(conf::tasks::PORTAL_CONFIG_TIMEOUT).count());
    wifiManager.setConnectRetries(3);  // 3 retries
    wifiManager.setConnectTimeout(10); // 10 seconds
    wifiManager.setCountry("IT");
    wifiManager.setTitle(strings::PORTAL_TITLE);
    wifiManager.setCaptivePortalEnable(true);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    if (force_reset)
    {
      wifiManager.resetSettings();
    }

#if (PINS_WOKWI)
    wifiManager.setDebugOutput(true);
    wifiManager.resetSettings();
    wifiManager.setTimeout(10); // fail fast for debugging
#endif
    auto must_skip = disable_portal || config.disablePortal;
    if (must_skip)
    {
      wifiManager.setTimeout(1);
    }

    if (wifiManager.autoConnect())
    {
      Board::logic.changeStatus(Status::PortalSuccess);
      delay(1000);
    }
    else
    {
      if (!must_skip)
      {
        Board::logic.changeStatus(Status::PortalFailed);
        delay(3000);
      }
    }

    if (shouldSaveConfig)
    {
      // save SSID data from WiFiManager
      config.ssid.assign(WiFi.SSID().c_str());
      config.password.assign(WiFi.psk().c_str());

      // read updated parameters
      config.mqtt_server.assign(custom_mqtt_server.getValue());
      config.mqtt_switch_topic.assign(custom_mqtt_topic.getValue());
      config.machine_id.assign(custom_machine_id.getValue());

      config.disablePortal = true;

      // save the custom parameters to EEPROM
      if (config.SaveToEEPROM())
      {
        ESP_LOGD(TAG, "Config saved to EEPROM");
      }
      else
      {
        ESP_LOGE(TAG, "Failed to save config to EEPROM");
      }

      // WiFi settings change may require full reboot
      Board::logic.setRebootRequest(true);
    }
  }

  void OTAComplete()
  {
    ESP_LOGI(TAG, "OTA complete, reboot requested");
    Board::logic.setRebootRequest(true);
  }

  void setupOTA()
  {
    ArduinoOTA.setHostname(Board::logic.getHostname().c_str());
    ArduinoOTA.onStart([]()
                       { Board::logic.changeStatus(Status::OTAStarting); });
    ArduinoOTA.onEnd(OTAComplete);
    ArduinoOTA.onError([](ota_error_t error)
                       { Board::logic.changeStatus(Status::OTAError); });
    ArduinoOTA.setMdnsEnabled(true);
    ArduinoOTA.setRebootOnSuccess(false);
    ArduinoOTA.setTimeout(45000);
    ArduinoOTA.begin();
  }
} // namespace fabomatic