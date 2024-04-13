#ifndef WHITELIST_HPP
#define WHITELIST_HPP

#include <array>
#include <string_view>
#include <tuple>
#include <unordered_map>

#include "FabUser.hpp"
#include "conf.hpp"

namespace fablabbg
{
  namespace conf::cards
  {
    static constexpr auto LEN = 10U; /* Number of whitelisted cards */
  }

  using WhiteListEntry = std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>;
  using WhiteList = std::array<WhiteListEntry, conf::cards::LEN>;
  using CachedList = std::array<FabUser, conf::rfid_tags::CACHE_LEN>;

  template <typename T, size_t N>
  bool ScrambledEquals(const std::array<T, N> &arr1, const std::array<T, N> &arr2)
  {
    std::array<T, N> sorted1 = arr1;
    std::array<T, N> sorted2 = arr2;
    std::sort(sorted1.begin(), sorted1.end());
    std::sort(sorted2.begin(), sorted2.end());

    return sorted1 == sorted2;
  }
}

#endif // WHITELIST_HPP
