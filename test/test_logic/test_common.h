#ifndef TEST_COMMON_H_
#define TEST_COMMON_H_

#include <optional>
#include <chrono>

#include "card.hpp"
#include "BoardLogic.hpp"
#include "BaseRfidWrapper.hpp"

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
  BoardLogic::Status simulate_rfid_card(BaseRFIDWrapper &rfid, BoardLogic &logic,
                                        std::optional<card::uid_t> uid,
                                        milliseconds duration_tap = milliseconds(250));

  void machine_init(BoardLogic &logic, BaseRFIDWrapper &rfid);
}

#endif // TEST_COMMON_H_