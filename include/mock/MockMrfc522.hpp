#ifndef _MOCKMRFC522_HPP_
#define _MOCKMRFC522_HPP_

#include <memory>
#include <optional>
#include "Arduino.h"

#include "MFRC522v2.h"
#include "MFRC522DriverSPI.h"
#include "MFRC522DriverPinSimple.h"
#include "FabUser.hpp"
#include "pins.hpp"

namespace fablabbg
{
  class MockMrfc522
  {
  private:
    std::optional<card::uid_t> uid;
    std::optional<time_point<system_clock>> stop_uid_simulate_time;
    std::optional<card::uid_t> getSimulatedUid() const;
    
  public:
    struct UidDriver
    {
      byte size; // Number of bytes in the UID. 4, 7 or 10.
      byte uidByte[10];
      byte sak; // The SAK (Select acknowledge) byte returned from the PICC after successful selection.
    };

    MockMrfc522();
    bool PICC_IsNewCardPresent();
    bool PICC_ReadCardSerial();
    void reset();
    bool PCD_Init();
    bool PICC_WakeupA(byte *bufferATQA, byte &bufferSize);
    bool PCD_PerformSelfTest();
    MockMrfc522::UidDriver getDriverUid() const;
    void PCD_SetAntennaGain(MFRC522Constants::PCD_RxGain gain);
    void PCD_DumpVersionToSerial();

    void setUid(const std::optional<card::uid_t> &uid, const std::optional<milliseconds> &max_delay);
    void resetUid();

    static constexpr auto RxGainMax = MFRC522::PCD_RxGain::RxGain_max;
  };
} // namespace fablabbg

#endif // _MOCKMRFC522_HPP_