#include <memory>

#include "mock/MockRFIDWrapper.hpp"
#include "pins.hpp"
#include "conf.hpp"
#include "card.hpp"
#include "secrets.hpp"

namespace fablabbg
{
  /// @brief indicates if a new card is present in the RFID chip antenna area
  /// @return true if a new card is present
  bool MockRFIDWrapper::isNewCardPresent() const
  {
    return fakeUid.has_value();
  }

  /// @brief tries to read the card serial number
  /// @return true if successfull, result can be read with getUid()
  std::optional<card::uid_t> MockRFIDWrapper::readCardSerial() const
  {
    return fakeUid;
  }

  /// @brief indicates if the card is still present in the RFID chip antenna area
  /// @param original the card ID to check
  bool MockRFIDWrapper::cardStillThere(const card::uid_t original) const
  {
    static constexpr auto NB_TRIES = 3;
    for (auto i = 0; i < NB_TRIES; i++)
    {
      // Detect Tag without looking for collisions
      if (readCardSerial() && getUid() == original)
        return true;
      delay(5);
    }
    return false;
  }

  /// @brief Performs a RFID chip self test
  /// @return true if successfull
  bool MockRFIDWrapper::selfTest() const
  {
    return true;
  }

  /// @brief Performs a chip reset
  void MockRFIDWrapper::reset() const {};

  /// @brief Transforms the RFID acquired bytes into a uid_id object
  /// @return card ID
  card::uid_t MockRFIDWrapper::getUid() const
  {
    if (fakeUid.has_value())
      return fakeUid.value();

    uint8_t arr[conf::rfid_tags::UID_BYTE_LEN]{0};
    auto [uid, level, nane] = secrets::cards::whitelist[0];

    memcpy(arr, &uid, conf::rfid_tags::UID_BYTE_LEN);

    auto c = card::from_array(arr);

    return c;
  }

  /// @brief Initializes RFID chip including self test
  bool MockRFIDWrapper::init_rfid() const
  {
    return true;
  }

  /// @brief Sets the card ID to be returned by the RFID chip
  /// @param original the card ID to set
  void MockRFIDWrapper::setUid(const card::uid_t original)
  {
    fakeUid = original;
  }

  void MockRFIDWrapper::resetUid()
  {
    fakeUid = std::nullopt;
  }
} // namespace fablabbg