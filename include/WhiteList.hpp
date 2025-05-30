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
    inline constexpr auto LEN = 10U; /* Number of whitelisted cards */
  } // namespace conf::cards

  using WhiteListEntry = std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>;
  using WhiteList = std::array<WhiteListEntry, conf::cards::LEN>;

} // namespace fabomatic

#endif // WHITELIST_HPP
