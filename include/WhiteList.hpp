#ifndef WHITELIST_HPP
#define WHITELIST_HPP

#include <array>
#include <string_view>
#include <tuple>

#include "FabUser.hpp"
#include "secrets.hpp"

namespace fablabbg
{
  namespace conf::cards
  {
    static constexpr auto LEN = 10U; /* Number of whitelisted cards */
  }

  using WhiteListEntry = std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>;
  using WhiteList = std::array<WhiteListEntry, conf::cards::LEN>;
  using CachedList = std::array<WhiteListEntry, conf::rfid_tags::CACHE_LEN>;
  
}

#endif // WHITELIST_HPP
