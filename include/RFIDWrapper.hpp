#ifndef RFIDWRAPPER_H_
#define RFIDWRAPPER_H_

#include <string>
#include <memory>
#include <optional>
#include <chrono>

#include "conf.hpp"
#include "card.hpp"
#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "BaseRfidWrapper.hpp"

namespace fablabbg
{
  using namespace std::chrono;

  class RFIDWrapper : public BaseRFIDWrapper
  {
  private:
    std::unique_ptr<MFRC522> mfrc522;
    std::unique_ptr<MFRC522DriverPinSimple> rfid_simple_driver;
    std::unique_ptr<MFRC522DriverSPI> spi_rfid_driver;

  public:
    RFIDWrapper();

    bool init_rfid() const;
    bool isNewCardPresent() const;
    bool cardStillThere(const card::uid_t original, milliseconds max_delay) const;
    std::optional<card::uid_t> readCardSerial() const;
    bool selfTest() const;
    void reset() const;
    card::uid_t getUid() const;

    RFIDWrapper(const RFIDWrapper &) = delete;             // copy constructor
    RFIDWrapper &operator=(const RFIDWrapper &x) = delete; // copy assignment
    RFIDWrapper(RFIDWrapper &&) = delete;                  // move constructor
    RFIDWrapper &operator=(RFIDWrapper &&) = delete;       // move assignment
  };
} // namespace fablabbg
#endif // RFIDWRAPPER_H_