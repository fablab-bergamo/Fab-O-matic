#ifndef RFIDWRAPPER_H_
#define RFIDWRAPPER_H_

#include <string>
#include <memory>
#include <optional>
#include <chrono>

#include "conf.hpp"
#include "card.hpp"
#include "BaseRfidWrapper.hpp"

namespace fablabbg
{
  using namespace std::chrono;

  template <typename Driver>
  class RFIDWrapper final : public BaseRFIDWrapper
  {
  private:
    std::unique_ptr<Driver> driver;

  public:
    RFIDWrapper();

    /// @brief Initializes the RFID driver
    bool init_rfid() const;

    /// @brief Returns true if a card is present in the field
    bool isNewCardPresent() const;

    /// @brief Returns true if the card is in the field, waiting up to max_delay to confirm presence
    /// @details This function may return immediately if the card responds quickly
    /// @param original card to look for
    /// @param max_delay maximum delay to contact the card
    /// @return true if the card is found
    bool cardStillThere(const card::uid_t original, milliseconds max_delay) const;

    /// @brief Reads the card serial number
    /// @return std::nullopt if the card is not present, the card serial otherwise
    std::optional<card::uid_t> readCardSerial() const;

    bool selfTest() const;

    void reset() const;
    card::uid_t getUid() const;

    /// @brief Returns the driver object for testing/simulation
    Driver &getDriver();

    RFIDWrapper(const RFIDWrapper &) = delete;             // copy constructor
    RFIDWrapper &operator=(const RFIDWrapper &x) = delete; // copy assignment
    RFIDWrapper(RFIDWrapper &&) = delete;                  // move constructor
    RFIDWrapper &operator=(RFIDWrapper &&) = delete;       // move assignment
  };
} // namespace fablabbg

#include "RFIDWrapper.tpp"

#endif // RFIDWRAPPER_H_