#ifndef WHITELIST_HPP
#define WHITELIST_HPP

#include <array>
#include <string_view>
#include <tuple>
#include <unordered_map>

#include "FabUser.hpp"
#include "conf.hpp"

namespace fabomatic
{
  namespace conf::cards
  {
    static constexpr auto LEN = 10U; /* Number of whitelisted cards */
  } // namespace conf::cards

  using WhiteListEntry = std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>;
  using WhiteList = std::array<WhiteListEntry, conf::cards::LEN>;

  struct CachedFabUser
  {
    card::uid_t uid;
    FabUser::UserLevel level;
    constexpr CachedFabUser() : uid(card::INVALID), level(FabUser::UserLevel::Unknown){};
    constexpr CachedFabUser(card::uid_t uid, FabUser::UserLevel level) : uid(uid), level(level){};
    constexpr auto operator<(const CachedFabUser &rhs) const -> bool
    {
      return uid < rhs.uid;
    }
    constexpr auto operator==(const CachedFabUser &rhs) const -> bool
    {
      return uid == rhs.uid;
    }
  };

  using CachedList = std::array<CachedFabUser, conf::rfid_tags::CACHE_LEN>;

  template <typename T, size_t N>
  [[nodiscard]] bool ScrambledEquals(const std::array<T, N> &arr1, const std::array<T, N> &arr2)
  {
    std::array<T, N> sorted1 = arr1;
    std::array<T, N> sorted2 = arr2;
    std::sort(sorted1.begin(), sorted1.end());
    std::sort(sorted2.begin(), sorted2.end());

    return sorted1 == sorted2;
  }
} // namespace fabomatic

#endif // WHITELIST_HPP
