#ifndef MOCKRFIDWRAPPER_H_
#define MOCKRFIDWRAPPER_H_

#include <string>
#include <memory>
#include <optional>

#include "conf.hpp"
#include "card.hpp"
#include "BaseRfidWrapper.hpp"

namespace fablabbg
{
  class MockRFIDWrapper : public BaseRFIDWrapper
  {
  public:
    MockRFIDWrapper() = default;

    bool init_rfid() const;
    bool isNewCardPresent() const;
    bool cardStillThere(const card::uid_t original) const;
    void setUid(const card::uid_t original);
    std::optional<card::uid_t> readCardSerial() const;
    bool selfTest() const;
    void reset() const;
    card::uid_t getUid() const;
    void resetUid();

    MockRFIDWrapper(const MockRFIDWrapper &) = delete;             // copy constructor
    MockRFIDWrapper &operator=(const MockRFIDWrapper &x) = delete; // copy assignment
    MockRFIDWrapper(MockRFIDWrapper &&) = delete;                  // move constructor
    MockRFIDWrapper &operator=(MockRFIDWrapper &&) = delete;       // move assignment

  private:
    std::optional<card::uid_t> fakeUid;
  };
} // namespace fablabbg
#endif // MOCKRFIDWRAPPER_H_