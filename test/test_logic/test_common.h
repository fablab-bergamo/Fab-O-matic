#ifndef TEST_COMMON_H_
#define TEST_COMMON_H_

#include <chrono>
#include <optional>

#include "BoardLogic.hpp"
#include "RFIDWrapper.hpp"
#include "card.hpp"
#include "mock/MockMrfc522.hpp"

namespace fabomatic::tests
{
  static constexpr WhiteList test_whitelist /* List of RFID tags whitelisted, regardless of connection */
      {
          std::make_tuple(0xAABBCCD1, FabUser::UserLevel::FabAdmin, "ABCDEFG"),
          std::make_tuple(0xAABBCCD2, FabUser::UserLevel::FabStaff, "PIPPO"),
          std::make_tuple(0xAABBCCD3, FabUser::UserLevel::NormalUser, "USER1"),
          std::make_tuple(0xAABBCCD4, FabUser::UserLevel::NormalUser, "USER2"),
          std::make_tuple(0xAABBCCD5, FabUser::UserLevel::NormalUser, "USER3"),
          std::make_tuple(0xAABBCCD7, FabUser::UserLevel::NormalUser, "USER4"),
          std::make_tuple(0xAABBCCD8, FabUser::UserLevel::NormalUser, "USER5"),
          std::make_tuple(0xAABBCCD9, FabUser::UserLevel::NormalUser, "USER6"),
          std::make_tuple(0xAABBCCD0, FabUser::UserLevel::NormalUser, "USER7"),
          std::make_tuple(0xAABBCCDA, FabUser::UserLevel::NormalUser, "USER8"),
      };

  /// @brief Simulates RFID card tap
  /// @param rfid RFID wrapper
  /// @param logic Board logic
  /// @param uid UID of the card to simulate or std::nullopt for no card
  /// @param duration_tap Duration of the tap
  BoardLogic::Status simulate_rfid_card(RFIDWrapper<MockMrfc522> &rfid, BoardLogic &logic, std::optional<card::uid_t> uid,
                                        std::optional<std::chrono::milliseconds> duration_tap = std::nullopt);

  void machine_init(BoardLogic &logic, RFIDWrapper<MockMrfc522> &rfid);

  constexpr fabomatic::card::uid_t get_test_uid(size_t idx)
  {
    auto [card_uid, level, name] = fabomatic::tests::test_whitelist[idx];
    return card_uid;
  }
}

#endif // TEST_COMMON_H_