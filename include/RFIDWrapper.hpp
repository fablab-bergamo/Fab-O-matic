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
  class RFIDWrapper : public BaseRFIDWrapper
  {
  private:
    std::unique_ptr<Driver> driver;

  public:
    RFIDWrapper();

    bool init_rfid() const;
    bool isNewCardPresent() const;
    bool cardStillThere(const card::uid_t original, milliseconds max_delay) const;
    std::optional<card::uid_t> readCardSerial() const;

    bool selfTest() const;

    void reset() const;
    card::uid_t getUid() const;

    // Testing methods
    void resetUid();
    void setUid(const card::uid_t &uid);

    RFIDWrapper(const RFIDWrapper &) = delete;             // copy constructor
    RFIDWrapper &operator=(const RFIDWrapper &x) = delete; // copy assignment
    RFIDWrapper(RFIDWrapper &&) = delete;                  // move constructor
    RFIDWrapper &operator=(RFIDWrapper &&) = delete;       // move assignment
  };
} // namespace fablabbg

#include "RFIDWrapper.tpp"

#endif // RFIDWRAPPER_H_