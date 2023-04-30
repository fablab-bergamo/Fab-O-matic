#ifndef MOCKRFIDWRAPPER_H_
#define MOCKRFIDWRAPPER_H_

#include <string>
#include <memory>

#include "conf.h"
#include "card.h"

class MockRFIDWrapper
{
private:
  mutable int card_idx = 0;

public:
  MockRFIDWrapper();

  bool init() const;
  bool isNewCardPresent() const;
  bool cardStillThere(const card::uid_t original) const;
  bool readCardSerial() const;
  bool selfTest() const;
  void reset() const;
  card::uid_t getUid() const;

  MockRFIDWrapper(const MockRFIDWrapper &) = delete;             // copy constructor
  MockRFIDWrapper &operator=(const MockRFIDWrapper &x) = delete; // copy assignment
  MockRFIDWrapper(MockRFIDWrapper &&) = delete;                  // move constructor
  MockRFIDWrapper &operator=(MockRFIDWrapper &&) = delete;       // move assignment
};

#endif // MOCKRFIDWRAPPER_H_