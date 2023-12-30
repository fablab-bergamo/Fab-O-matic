#include <memory>

#include "RFIDWrapper.hpp"
#include "pins.hpp"
#include "conf.hpp"
#include "card.hpp"

namespace fablabbg
{
  /// @brief Constructor
  template <typename Driver>
  RFIDWrapper<Driver>::RFIDWrapper() : driver{std::make_unique<Driver>()} {};

  /// @brief indicates if a new card is present in the RFID chip antenna area
  /// @return true if a new card is present
  template <typename Driver>
  bool RFIDWrapper<Driver>::isNewCardPresent() const
  {
    auto result = driver->PICC_IsNewCardPresent();

    if (conf::debug::ENABLE_LOGS && result)
      Serial.printf("isNewCardPresent=%d\r\n", result);

    return result;
  }

  /// @brief tries to read the card serial number
  /// @return true if successfull, result can be read with getUid()
  template <typename Driver>
  std::optional<card::uid_t> RFIDWrapper<Driver>::readCardSerial() const
  {
    auto result = driver->PICC_ReadCardSerial();
    const auto &uid = driver->getDriverUid();

    if (conf::debug::ENABLE_LOGS)
    {
      Serial.printf("readCardSerial=%d (SAK=%d, Size=%d)\r\n", result,
                    uid.sak, uid.size);
    }
    if (result)
    {
      return getUid();
    }
    else
    {
      return std::nullopt;
    }
  }

  /// @brief indicates if the card is still present in the RFID chip antenna area
  /// @param original the card ID to check
  template <typename Driver>
  bool RFIDWrapper<Driver>::cardStillThere(const card::uid_t original, milliseconds max_delay) const
  {
    auto start = std::chrono::system_clock::now();
    do
    {
      // Detect Tag without looking for collisions
      byte bufferATQA[2];
      byte bufferSize = sizeof(bufferATQA);

      if (driver->PICC_WakeupA(bufferATQA, bufferSize))
      {
        if (readCardSerial() == original)
          return true;
      }
      delay(20);
    } while (std::chrono::system_clock::now() - start < max_delay);

    return false;
  }

  /// @brief Performs a RFID chip self test
  /// @return true if successfull
  template <typename Driver>
  bool RFIDWrapper<Driver>::selfTest() const
  {
    auto result = driver->PCD_PerformSelfTest();
    if (conf::debug::ENABLE_LOGS)
    {
      Serial.printf("RFID self test = %d\r\n", result);
    }
    return result;
  }

  /// @brief Performs a chip reset
  template <typename Driver>
  void RFIDWrapper<Driver>::reset() const
  {
    digitalWrite(pins.mfrc522.reset_pin, 1);
    delay(25);
    digitalWrite(pins.mfrc522.reset_pin, 0);
    delay(25);
  }

  /// @brief Transforms the RFID acquired bytes into a uid_id object
  /// @return card ID
  template <typename Driver>
  card::uid_t RFIDWrapper<Driver>::getUid() const
  {
    uint8_t arr[conf::rfid_tags::UID_BYTE_LEN]{0};
    auto const &uid = driver->getDriverUid();
    memcpy(arr, uid.uidByte, std::min(conf::rfid_tags::UID_BYTE_LEN, uid.size));

    auto c = card::from_array(arr);

    return c;
  }

  /// @brief Initializes RFID chip including self test
  template <typename Driver>
  bool RFIDWrapper<Driver>::init_rfid() const
  {

    if (conf::debug::ENABLE_LOGS)
    {
      constexpr auto MAX_LEN = 80;
      char buffer[MAX_LEN] = {0};
      if (snprintf(buffer, sizeof(buffer), "Configuring SPI RFID (SCK=%d, MISO=%d, MOSI=%d, SDA=%d) RESET=%d",
                   pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin,
                   pins.mfrc522.sda_pin, pins.mfrc522.reset_pin) > 0)
        Serial.println(buffer);
    }

    reset();

    if (!driver->PCD_Init())
    {
      Serial.println("mfrc522 Init failed");
      return false;
    }

    if (conf::debug::ENABLE_LOGS)
      driver->PCD_DumpVersionToSerial();

    driver->PCD_SetAntennaGain(Driver::RxGainMax);
    delay(5);

    if (!driver->PCD_PerformSelfTest())
    {
      Serial.println("Self-test failure for RFID");
      return false;
    }
    return true;
  }

  /// @brief Sets the card ID for testing
  /// @param uid the card ID
  template <typename Driver>
  void RFIDWrapper<Driver>::setUid(const card::uid_t &uid)
  {
    if (conf::debug::ENABLE_LOGS)
    {
      auto str_id = card::uid_str(uid);
      Serial.printf("setUid=%s\r\n", str_id.c_str());
    }

    driver->setUid(uid);
  }

  /// @brief Resets the card ID for testing
  template <typename Driver>
  void RFIDWrapper<Driver>::resetUid()
  {
    if (conf::debug::ENABLE_LOGS)
      Serial.println("resetUid");

    driver->resetUid();
  }
} // namespace fablabbg