#include <sstream>

#include "Arduino.h"
#include "WiFi.h"
#include <Adafruit_NeoPixel.h>

#include "AuthProvider.hpp"
#include "BaseRfidWrapper.hpp"
#include "BoardLogic.hpp"
#include "FabBackend.hpp"
#include "Logging.hpp"
#include "Machine.hpp"
#include "SavedConfig.hpp"
#include "Tasks.hpp"
#include "conf.hpp"
#include "pins.hpp"
#include "secrets.hpp"
#include "language/lang.hpp"

#ifndef GIT_VERSION
#define GIT_VERSION "??????"
#endif
#include <driver/gpio.h>

namespace fabomatic
{
  using milliseconds = std::chrono::milliseconds;

  BaseRFIDWrapper &BoardLogic::getRfid() const
  {
    if (rfid.has_value())
      return rfid.value().get();

    ESP_LOGE(TAG, "RFID not initialized");
    while (true)
    {
      Tasks::delay(1s);
    };
  }

  /// @brief connects and polls the server for up-to-date machine information
  void BoardLogic::refreshFromServer()
  {
    ESP_LOGD(TAG, "BoardLogic::refreshFromServer() called");

    if (server.connect())
    {
      // Check the configured machine data from the server
      const auto result = server.checkMachine();
      if (result->request_ok)
      {
        if (result->is_valid)
        {
          machine.setMaintenanceNeeded(result->maintenance);
          machine.setAllowed(result->allowed);
          machine.setAutologoffDelay(std::chrono::minutes(result->logoff));
          machine.setGracePeriod(std::chrono::minutes(result->grace));
          machine.setMachineName(result->name);
          machine.setMaintenanceInfo(result->description);
          MachineType mt = static_cast<MachineType>(result->type);
          machine.setMachineType(mt);

          ESP_LOGD(TAG, "Machine data updated:%s", machine.toString().c_str());
        }
        else
        {
          ESP_LOGW(TAG, "The configured machine ID %u is unknown to the server\r\n", machine.getMachineId().id);
        }
      }
    }
  }

  /// @brief Called when a RFID tag has been detected
  void BoardLogic::onNewCard(card::uid_t uid)
  {
    ESP_LOGD(TAG, "New card present");

    if (!ready_for_a_new_card)
    {
      return;
    }
    ready_for_a_new_card = false;

    if (machine.isFree())
    {
      // machine is free
      if (!authorize(uid))
      {
        ESP_LOGI(TAG, "Login failed for %s", card::uid_str(uid).c_str());
      }
      Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY);
      refreshFromServer();
      return;
    }

    // machine is busy
    if (machine.getActiveUser().card_uid == uid)
    {
      // we can logout. we should require that the card stays in the field for some seconds, to prevent accidental logout. maybe sound a buzzer?
      logout();
    }
    else
    {
      // user is not the same, display who is using it
      changeStatus(Status::AlreadyInUse);
    }
    Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY);
    return;
  }

  /// @brief Removes the current machine user and changes the status to LoggedOut
  void BoardLogic::logout()
  {
    const auto result = server.finishUse(machine.getActiveUser().card_uid,
                                         machine.getUsageDuration());

    ESP_LOGI(TAG, "Logout, result finishUse: %d", result->request_ok);

    machine.logout();
    changeStatus(Status::LoggedOut);
    beepOk();
    Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY);
  }

  /// @brief Asks the user to keep the RFID tag on the reader as confirmation
  /// @return True if the user accepted, False if user bailed out
  bool BoardLogic::longTap(const card::uid_t card, const std::string &short_prompt) const
  {
    constexpr auto STEPS_COUNT = 6;
    constexpr milliseconds delay_per_step = std::chrono::duration_cast<std::chrono::milliseconds>(conf::machine::LONG_TAP_DURATION) / STEPS_COUNT;
    const BoardInfo bi = {server.isOnline(), machine.getPowerState(), machine.isShutdownImminent()};

    for (auto step = 0; step < STEPS_COUNT; step++)
    {
      std::stringstream ss;
      ss << short_prompt << " " << step << "/" << STEPS_COUNT;
      getLcd().setRow(1, ss.str());
      getLcd().update(bi);

      const auto start = fabomatic::Tasks::arduinoNow();
      if (!getRfid().cardStillThere(card, delay_per_step))
      {
        getLcd().setRow(1, strings::S_CANCELLED);
        getLcd().update(bi);
        return false;
      }

      // cardStillThere may have returned immediately, so we need to wait a bit
      const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(fabomatic::Tasks::arduinoNow() - start);
      if (delay_per_step - elapsed > 10ms)
      {
        Tasks::delay(delay_per_step - elapsed);
      }
    }

    getLcd().setRow(1, strings::S_CONFIRMED);
    getLcd().update(bi);
    return true;
  }

  /// @brief Checks if the card UID is valid, and tries to check the user in to the machine.
  /// @param uid card uid
  /// @return true if the user is now logged on to the machine
  bool BoardLogic::authorize(card::uid_t uid)
  {
    changeStatus(Status::Verifying);
    FabUser user;

    user.authenticated = false;
    user.holder_name = "?";
    user.card_uid = uid;
    user.user_level = FabUser::UserLevel::Unknown;

    const auto response = auth.tryLogin(uid, server);
    if (!response.has_value() || response.value().user_level == FabUser::UserLevel::Unknown)
    {
      ESP_LOGI(TAG, "Failed login for %s", card::uid_str(uid).c_str());
      changeStatus(Status::LoginDenied);
      beepFail();
      return false;
    }

    user = response.value();

    if (!machine.isAllowed())
    {
      ESP_LOGI(TAG, "Login refused due to machine not allowed");
      changeStatus(Status::NotAllowed);
      beepFail();
      return false;
    }

    if (machine.isMaintenanceNeeded())
    {
      if (conf::machine::MAINTENANCE_BLOCK &&
          user.user_level < FabUser::UserLevel::FabStaff)
      {
        changeStatus(Status::MaintenanceNeeded);
        beepFail();
        Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY);
        return false;
      }
      if (user.user_level >= FabUser::UserLevel::FabStaff)
      {
        beepOk();
        changeStatus(Status::MaintenanceQuery);

        if (longTap(user.card_uid, strings::S_LONGTAP_PROMPT))
        {
          const auto maint_resp = server.registerMaintenance(user.card_uid);
          if (!maint_resp->request_ok)
          {
            beepFail();
            changeStatus(Status::Error);
            Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY);
            // Allow bypass for admins
            if (user.user_level == FabUser::UserLevel::FabAdmin)
            {
              machine.setMaintenanceNeeded(false);
            }
          }
          else
          {
            changeStatus(Status::MaintenanceDone);
            machine.setMaintenanceNeeded(false);
            beepOk();
            Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY * 2);
          }
          // Proceed to log-on the staff member to the machine in all cases
        }
      }
    }

    if (machine.login(user))
    {
      const auto result = server.startUse(machine.getActiveUser().card_uid);
      ESP_LOGI(TAG, "Login, result startUse: %d", result->request_ok);
      changeStatus(Status::LoggedIn);
      beepOk();
    }
    else
    {
      changeStatus(Status::NotAllowed);
      beepFail();
      Tasks::delay(conf::lcd::SHORT_MESSAGE_DELAY);
    }

    return true;
  }

  /// @brief Initializes LCD and RFID classes
  bool BoardLogic::initBoard()
  {
    ESP_LOGD(TAG, "Board initialization...");

    auto success = getLcd().begin();
    success &= getRfid().rfidInit();

    // Setup buzzer pin for ESP32
    if constexpr (pins.buzzer.pin != NO_PIN)
    {
      pinMode(pins.buzzer.pin, OUTPUT);
      gpio_set_drive_capability(static_cast<gpio_num_t>(pins.buzzer.pin), GPIO_DRIVE_CAP_2);
    }

    ESP_LOGI(TAG, "Board initialization complete, success = %d", success);

    return success;
  }

  /// @brief Sets the board in the state given.
  /// @param new_state new state
  void BoardLogic::changeStatus(Status new_state)
  {
    if (status != new_state)
    {
      ESP_LOGI(TAG, "** Changing board state to %d", static_cast<int>(new_state));
    }

    status = new_state;
    updateLCD();
  }

  /// @brief Updates the LCD screen as per the current status
  void BoardLogic::updateLCD() const
  {
    std::stringstream buffer; // Null terminated strings
    std::string user_name, machine_name, uid_str;

    auto &lcd = getLcd();
    lcd.showConnection(true);
    lcd.showPower(true);

    user_name = machine.getActiveUser().holder_name;
    machine_name = machine.isConfigured() ? machine.getMachineName() : "-";

    if (rfid.has_value())
    {
      auto uid = rfid.value().get().getUid();
      uid_str = card::uid_str(uid);
    }
    else
    {
      uid_str = "????????";
    }
    std::stringstream idv;
    idv << "ID:" << this->getMachine().getMachineId()
        << " V" << GIT_VERSION;

    switch (status)
    {
    case Status::Clear:
      lcd.clear();
      break;
    case Status::MachineFree:
      lcd.setRow(0, machine_name);
      if (!machine.isAllowed())
      {
        lcd.setRow(1, strings::S_MACHINE_BLOCKED);
      }
      else if (machine.isMaintenanceNeeded())
      {
        lcd.setRow(0, strings::S_MACHINE_MAINTENANCE);
        lcd.setRow(1, machine.getMaintenanceInfo());
      }
      else
      {
        lcd.setRow(1, strings::S_CARD_PROMPT);
      }
      break;
    case Status::AlreadyInUse:
      lcd.setRow(0, strings::S_USED_BY);
      lcd.setRow(1, user_name);
      break;
    case Status::LoggedIn:
      lcd.setRow(0, strings::S_START_USE);
      lcd.setRow(1, user_name);
      break;
    case Status::LoginDenied:
      lcd.setRow(0, strings::S_LOGIN_DENIED);
      lcd.setRow(1, uid_str);
      break;
    case Status::LoggedOut:
      lcd.setRow(0, strings::S_GOODBYE);
      lcd.setRow(1, user_name);
      break;
    case Status::Connecting:
      lcd.setRow(0, strings::S_CONNECTING_MQTT_1);
      lcd.setRow(1, strings::S_CONNECTING_MQTT_2);
      break;
    case Status::Connected:
      lcd.setRow(0, strings::S_CONNECTED);
      lcd.setRow(1, "");
      break;
    case Status::MachineInUse:
      buffer << strings::S_HELLO << " " << user_name;
      lcd.setRow(0, buffer.str());
      lcd.setRow(1, lcd.convertSecondsToHHMMSS(machine.getUsageDuration()));
      break;
    case Status::Busy:
      lcd.setRow(0, strings::S_WORKING);
      lcd.setRow(1, "");
      break;
    case Status::Offline:
      lcd.setRow(0, strings::S_OFFLINE_MODE);
      lcd.setRow(1, "");
      break;
    case Status::NotAllowed:
      lcd.setRow(0, strings::S_BLOCKED_ADMIN_1);
      lcd.setRow(1, strings::S_BLOCKED_ADMIN_2);
      break;
    case Status::Verifying:
      lcd.setRow(0, strings::S_VERIFYING_1);
      lcd.setRow(1, strings::S_VERIFYING_2);
      break;
    case Status::MaintenanceNeeded:
      lcd.setRow(0, strings::S_BLOCKED_MAINTENANCE_1);
      lcd.setRow(1, strings::S_BLOCKED_MAINTENANCE_2);
      break;
    case Status::MaintenanceQuery:
      lcd.setRow(0, strings::S_PROMPT_MAINTENANCE_1);
      lcd.setRow(1, strings::S_PROMPT_MAINTENANCE_2);
      break;
    case Status::MaintenanceDone:
      lcd.setRow(0, strings::S_MAINTENANCE_REGISTERED_1);
      lcd.setRow(1, strings::S_MAINTENANCE_REGISTERED_2);
      break;
    case Status::Error:
      lcd.setRow(0, strings::S_GENERIC_ERROR);
      lcd.setRow(1, idv.str());
      break;
    case Status::ErrorHardware:
      lcd.setRow(0, strings::S_HW_ERROR);
      lcd.setRow(1, idv.str());
      break;
    case Status::PortalFailed:
      lcd.setRow(0, strings::S_PORTAL_ERROR);
      lcd.setRow(1, WiFi.softAPIP().toString().c_str());
      break;
    case Status::PortalSuccess:
      lcd.setRow(0, strings::S_PORTAL_SUCCESS);
      lcd.setRow(1, idv.str());
      break;
    case Status::PortalStarting:
      lcd.setRow(0, strings::S_OPEN_PORTAL);
      lcd.setRow(1, WiFi.softAPIP().toString().c_str());
      break;
    case Status::Booting:
      lcd.setRow(0, strings::S_BOOTING);
      lcd.setRow(1, idv.str());
      break;
    case Status::ShuttingDown:
      lcd.setRow(0, machine_name);
      lcd.setRow(1, strings::S_SHUTTING_DOWN);
      break;
    case Status::OTAStarting:
      lcd.setRow(0, strings::UPDATE_OTA_1);
      lcd.setRow(1, strings::UPDATE_OTA_2);
      break;
    case Status::FactoryDefaults:
      lcd.setRow(0, strings::FACTORY_RESET_DONE_1);
      lcd.setRow(1, strings::FACTORY_RESET_DONE_2);
      break;
    case Status::OTAError:
      lcd.setRow(0, strings::S_OTA_ERROR);
      lcd.setRow(1, "");
      break;
    default:
      lcd.setRow(0, strings::S_STATUS_ERROR_1);
      buffer << strings::S_STATUS_ERROR_2 << static_cast<int>(status);
      lcd.setRow(1, buffer.str());
      break;
    }
    BoardInfo bi = {server.isOnline(), machine.getPowerState(), machine.isShutdownImminent()};
    lcd.update(bi, false);
  }

  /// @brief Gets the current board status
  /// @return board status
  auto BoardLogic::getStatus() const -> BoardLogic::Status
  {
    return status;
  }

  void BoardLogic::beepOk() const
  {
    buzzer.beepOk();
  }

  void BoardLogic::beepFail() const
  {
    buzzer.beepFail();
  }

  /// @brief Configures the board with the given references
  bool BoardLogic::configure(BaseRFIDWrapper &rfid, LCDWrapper &lcd)
  {
    this->rfid = rfid;
    this->lcd = lcd;
    return reconfigure();
  }

  /// @brief Configures the board with the given references
  bool BoardLogic::reconfigure()
  {
    auto success = true;

    // Load configuration
    auto config = SavedConfig::LoadFromEEPROM();
    if (!config)
    {
      ESP_LOGW(TAG, "No configuration found in EEPROM, using defaults.");
      auto new_config = SavedConfig::DefaultConfig();
      success &= new_config.SaveToEEPROM();
      config = new_config;
    }

    ESP_LOGD(TAG, "Configuration found in EEPROM: %s", config->toString().c_str());

    server.configure(config.value());

    MachineID mid = config.value().getMachineID();
    MachineConfig machine_conf(mid,
                               conf::default_config::machine_type,
                               std::string{conf::default_config::machine_name},
                               pins.relay,
                               std::string{config.value().mqtt_switch_topic},
                               conf::machine::DEFAULT_AUTO_LOGOFF_DELAY,
                               conf::machine::DEFAULT_GRACE_PERIOD);

    machine.configure(machine_conf, server);
    buzzer.configure();
    auth.loadCache();

    return success;
  }

  /// @brief Blinks the LED
  void BoardLogic::blinkLed(uint8_t r, uint8_t g, uint8_t b)
  {
    led.set(Led::Status::Blinking);

    if (server.isOnline())
    {
      if (!machine.isAllowed() || machine.isMaintenanceNeeded())
      {
        led.setColor(64, 0, 0); // Red
      }
      else
      {
        if (machine.isFree())
        {
          if (machine.isShutdownImminent())
          {
            led.setColor(64, 0, 64); // Purple
          }
          else
          {
            led.setColor(0, 64, 0); // Green
          }
        }
        else
        {
          led.setColor(0, 0, 64); // Blue
        }
      }
    }
    else
    {
      led.setColor(128, 255, 0); // Orange
    }

    // User override
    if (r != 0 || g != 0 || b != 0)
    {
      led.setColor(r, g, b);
    }

    // Color override
    if (status == Status::ErrorHardware || status == Status::Error)
    {
      led.setColor(255, 0, 0); // Red
    }

    led.update();
  }

  /// @brief Checks if a new card is present
  void BoardLogic::checkRfid()
  {
    auto &rfid = getRfid();

    // check if there is a card
    if (rfid.isNewCardPresent())
    {
      const auto &result = rfid.readCardSerial();
      if (result)
      {
        onNewCard(result.value());
      }
      return;
    }

    // No new card present
    ready_for_a_new_card = true;
    if (machine.isFree())
    {
      changeStatus(Status::MachineFree);
    }
    else
    {
      changeStatus(Status::MachineInUse);
    }
  }

  /// @brief Checks if the machine must be powered off
  void BoardLogic::checkPowerOff()
  {
    if (machine.canPowerOff())
    {
      machine.power(false);
    }
  }

  /// @brief returns a modificable machine for testing only
  /// @return machine
  auto BoardLogic::getMachineForTesting() -> Machine &
  {
    return machine;
  }

  /// @brief returns a modificable machine for testing only
  /// @return a non-null Buzzer*
  auto BoardLogic::getBuzzerForTesting() -> Buzzer *
  {
    return &buzzer;
  }

  /// @brief Gets the current machine
  /// @return a machine object
  auto BoardLogic::getMachine() const -> const Machine &
  {
    return machine;
  }

  auto BoardLogic::getLcd() const -> LCDWrapper &
  {
    if (lcd.has_value())
    {
      return lcd.value().get();
    }
    else
    {
      ESP_LOGE(TAG, "LCD not initialized");
      while (true)
      {
        Tasks::delay(1s);
      };
    }
  }

  /// @brief Sets the autologoff delay
  /// @param delay new delay
  auto BoardLogic::setAutologoffDelay(std::chrono::seconds delay) -> void
  {
    machine.setAutologoffDelay(delay);
  }

  auto BoardLogic::setWhitelist(WhiteList whitelist) -> void
  {
    auth.setWhitelist(whitelist);
  }

  auto BoardLogic::getServer() -> FabBackend &
  {
    return server;
  }

  auto BoardLogic::setRebootRequest(bool request) -> void
  {
    rebootRequest = request;
  }

  auto BoardLogic::getRebootRequest() const -> bool
  {
    return rebootRequest;
  }

  auto BoardLogic::saveRfidCache() -> bool
  {
    return this->auth.saveCache();
  }

  auto BoardLogic::getHostname() const -> const std::string
  {
    // Hostname is BOARD + machine_id (which shall be unique) e.g. BOARD1
    return conf::default_config::hostname.data() +
           std::to_string(conf::default_config::machine_id.id);
  }

} // namespace fabomatic