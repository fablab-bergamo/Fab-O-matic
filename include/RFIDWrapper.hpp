#ifndef RFIDWRAPPER_HPP_
#define RFIDWRAPPER_HPP_

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "BaseRfidWrapper.hpp"
#include "card.hpp"
#include "conf.hpp"

namespace fablabbg
{
  template <typename Driver>
  class RFIDWrapper final : public BaseRFIDWrapper
  {
  private:
    std::unique_ptr<Driver> driver;

  public:
    RFIDWrapper();

    /// @brief Initializes the RFID driver
    [[nodiscard]] bool init_rfid() const override;

    /// @brief Returns true if a card is present in the field
    [[nodiscard]] bool isNewCardPresent() const override;

    /// @brief Returns true if the card is in the field, waiting up to max_delay to confirm presence
    /// @details This function may return immediately if the card responds quickly
    /// @param original card to look for
    /// @param max_delay maximum delay to contact the card
    /// @return true if the card is found
    [[nodiscard]] bool cardStillThere(const card::uid_t original, std::chrono::milliseconds max_delay) const override;

    /// @brief Reads the card serial number
    /// @return std::nullopt if the card is not present, the card serial otherwise
    [[nodiscard]] std::optional<card::uid_t> readCardSerial() const override;

    [[nodiscard]] bool selfTest() const override;

    void reset() const override;

    [[nodiscard]] card::uid_t getUid() const override;

    /// @brief Returns the driver object for testing/simulation
    Driver &getDriver();

    RFIDWrapper(const RFIDWrapper &) = delete;             // copy constructor
    RFIDWrapper &operator=(const RFIDWrapper &x) = delete; // copy assignment
    RFIDWrapper(RFIDWrapper &&) = delete;                  // move constructor
    RFIDWrapper &operator=(RFIDWrapper &&) = delete;       // move assignment
  };
} // namespace fablabbg

#include "RFIDWrapper.tpp"

#endif // RFIDWRAPPER_HPP_