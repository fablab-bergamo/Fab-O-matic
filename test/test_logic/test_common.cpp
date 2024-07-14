#include <chrono>
#include <optional>

#include "BoardLogic.hpp"
#include "RFIDWrapper.hpp"
#include "card.hpp"
#include "mock/MockMrfc522.hpp"
#include "test_common.h"
#include "unity.h"
#include "Tasks.hpp"

namespace fabomatic::tests
{
  /// @brief  Simulates RFID card tap
  /// @param rfid RFID wrapper for simulation
  /// @param logic Board logic, the checkRfid() method will be called repeatedly
  /// @param uid card UID to tap
  /// @param duration_tap duration of the tap. pass milliseconds::max() to keep the card in the field
  /// @return
  BoardLogic::Status simulate_rfid_card(RFIDWrapper<MockMrfc522> &rfid, BoardLogic &logic, std::optional<card::uid_t> uid,
                                        std::optional<std::chrono::milliseconds> duration_tap)
  {
    constexpr auto DEFAULT_CYCLES = 3;
    MockMrfc522 &driver = rfid.getDriver();
    driver.resetUid();
    for (auto i = 0; i < DEFAULT_CYCLES; i++)
    {
      logic.checkRfid();
    }
    if (uid.has_value())
    {
      driver.setUid(uid.value(), duration_tap);
      TEST_ASSERT_TRUE_MESSAGE(uid == rfid.getUid(), "Card UID not equal");
      auto start = fabomatic::Tasks::arduinoNow();

      do
      {
        logic.checkRfid();
        delay(50);
      } while (duration_tap.has_value() && fabomatic::Tasks::arduinoNow() - start < duration_tap);

    }
    else if (duration_tap)
    {
      delay(duration_tap.value().count());
    }
    return logic.getStatus();
  }

  /// @brief Resets machine to initial state, clearing flags
  void machine_init(BoardLogic &logic, RFIDWrapper<MockMrfc522> &rfid)
  {
    auto &machine = logic.getMachineForTesting();
    machine.setAllowed(true);
    machine.setMaintenanceNeeded(false);
    logic.logout();
    simulate_rfid_card(rfid, logic, std::nullopt);
    TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::MachineFree, "machine_init: Status not MachineFree");
    TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "machine_init: machine not free");
  }
}
