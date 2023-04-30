#include "MockRFIDWrapper.h"
#include "pins.h"
#include "conf.h"
#include "card.h"
#include <memory>
#include "secrets.h"

MockRFIDWrapper::MockRFIDWrapper()
{
}

/// @brief indicates if a new card is present in the RFID chip antenna area
/// @return true if a new card is present
bool MockRFIDWrapper::isNewCardPresent() const
{
  bool result = (random(0, 100) == 0);
  if (result)
  {
    // Choose a random card from the whitelist
    this->card_idx = random(0, conf::whitelist::LEN);
  }

  if (conf::debug::ENABLE_LOGS && result)
    Serial.printf("isNewCardPresent=%d\r\n", result);

  return result;
}

/// @brief tries to read the card serial number
/// @return true if successfull, result can be read with getUid()
bool MockRFIDWrapper::readCardSerial() const
{
  auto result = true;

  if (conf::debug::ENABLE_LOGS)
  {
    Serial.printf("readCardSerial=%d (SAK=%d, Size=%d)\r\n", result, 0, 4);
  }

  return result;
}

/// @brief indicates if the card is still present in the RFID chip antenna area
/// @param original the card ID to check
bool MockRFIDWrapper::cardStillThere(const card::uid_t original) const
{
  for (auto i = 0; i < 3; i++)
  {
    // Detect Tag without looking for collisions
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);

    if (true)
    {
      if (this->readCardSerial() && this->getUid() == original)
        return true;
    }
    delay(5);
  }
  return false;
}

/// @brief Performs a RFID chip self test
/// @return true if successfull
bool MockRFIDWrapper::selfTest() const
{
  auto result = true;
  if (conf::debug::ENABLE_LOGS)
  {
    Serial.printf("RFID self test = %d\r\n", result);
  }
  return result;
}

/// @brief Performs a chip reset
void MockRFIDWrapper::reset() const
{
  digitalWrite(pins.mfrc522.reset_pin, 1);
  delay(25);
  digitalWrite(pins.mfrc522.reset_pin, 0);
  delay(25);
}

/// @brief Transforms the RFID acquired bytes into a uid_id object
/// @return card ID
card::uid_t MockRFIDWrapper::getUid() const
{
  uint8_t arr[conf::whitelist::UID_BYTE_LEN]{0};
  auto [uid, level, nane] = secrets::cards::whitelist[this->card_idx];

  memcpy(arr, &uid, conf::whitelist::UID_BYTE_LEN);

  auto c = card::from_array(arr);

  if (conf::debug::ENABLE_LOGS)
  {
    auto str_id = card::uid_str(c);
    Serial.printf("getUid=%s\r\n", str_id.c_str());
  }

  return c;
}

/// @brief Initializes RFID chip including self test
bool MockRFIDWrapper::init() const
{

  if (conf::debug::ENABLE_LOGS)
  {
    constexpr auto MAX_LEN = 80;
    char buffer[MAX_LEN] = {0};
    if (sprintf(buffer, "Configuring Fake SPI RFID (SCK=%d, MISO=%d, MOSI=%d, SDA=%d) RESET=%d",
                pins.mfrc522.sck_pin, pins.mfrc522.miso_pin, pins.mfrc522.mosi_pin,
                pins.mfrc522.sda_pin, pins.mfrc522.reset_pin) > 0)
      Serial.println(buffer);
  }

  return true;
}