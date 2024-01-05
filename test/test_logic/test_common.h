#ifndef TEST_COMMON_H_
#define TEST_COMMON_H_

#include <optional>
#include <chrono>

#include "card.hpp"
#include "BoardLogic.hpp"
#include "mock/MockMrfc522.hpp"
#include "RFIDWrapper.hpp"

namespace fablabbg::tests
{
  using namespace fablabbg;
  using namespace std::chrono;

  static constexpr WhiteList test_whitelist /* List of RFID tags whitelisted, regardless of connection */
      {
          std::make_tuple(0xAABBCCD1, FabUser::UserLevel::FABLAB_ADMIN, "ABCDEFG"),
          std::make_tuple(0xAABBCCD2, FabUser::UserLevel::FABLAB_STAFF, "PIPPO"),
          std::make_tuple(0xAABBCCD3, FabUser::UserLevel::FABLAB_USER, "USER1"),
          std::make_tuple(0xAABBCCD4, FabUser::UserLevel::FABLAB_USER, "USER2"),
          std::make_tuple(0xAABBCCD5, FabUser::UserLevel::FABLAB_USER, "USER3")};

  /// @brief Simulates RFID card tap
  /// @param rfid RFID wrapper
  /// @param logic Board logic
  /// @param uid UID of the card to simulate or std::nullopt for no card
  /// @param duration_tap Duration of the tap
  BoardLogic::Status simulate_rfid_card(RFIDWrapper<MockMrfc522> &rfid, BoardLogic &logic, std::optional<card::uid_t> uid,
                                        std::optional<milliseconds> duration_tap = std::nullopt);

  void machine_init(BoardLogic &logic, RFIDWrapper<MockMrfc522> &rfid);
}

#endif // TEST_COMMON_H_