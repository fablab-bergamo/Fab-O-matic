#include <optional>
#include <chrono>

#include "unity.h"
#include "card.hpp"
#include "mock/MockRFIDWrapper.hpp"
#include "test_common.h"
#include "BoardLogic.hpp"

namespace fablabbg::tests
{
  BoardLogic::Status simulate_rfid_card(MockRFIDWrapper &rfid, BoardLogic &logic, std::optional<card::uid_t> uid,
                                        milliseconds duration_tap)
  {
    constexpr auto DEFAULT_CYCLES = 3;
    rfid.resetUid();
    for (auto i = 0; i < DEFAULT_CYCLES; i++)
    {
      logic.checkRfid();
    }
    if (uid.has_value())
    {
      rfid.setUid(uid.value());
      TEST_ASSERT_TRUE_MESSAGE(uid == rfid.getUid(), "Card UID not equal");
      auto start = system_clock::now();
      do
      {
        logic.checkRfid();
        delay(50);
      } while (system_clock::now() - start < duration_tap);
    }
    return logic.getStatus();
  }

  /// @brief Resets machine to initial state, clearing flags
  void machine_init(BoardLogic &logic, MockRFIDWrapper &rfid)
  {
    auto &machine = logic.getMachineForTesting();
    machine.allowed = true;
    machine.maintenanceNeeded = false;
    logic.logout();
    simulate_rfid_card(rfid, logic, std::nullopt);
    TEST_ASSERT_TRUE_MESSAGE(logic.getStatus() == BoardLogic::Status::FREE, "machine_init: Status not FREE");
    TEST_ASSERT_TRUE_MESSAGE(logic.getMachine().isFree(), "machine_init: machine not free");
  }
}
