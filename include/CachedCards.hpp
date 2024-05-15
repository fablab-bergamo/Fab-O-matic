#ifndef CACHEDCARDS_HPP
#define CACHEDCARDS_HPP

#include <array>

#include "FabUser.hpp"
#include "conf.hpp"
#include "card.hpp"

namespace fabomatic
{
  struct CachedCard
  {
    card::uid_t uid;
    FabUser::UserLevel level;
    constexpr CachedCard() : uid(card::INVALID), level(FabUser::UserLevel::Unknown){};
    constexpr CachedCard(card::uid_t uid, FabUser::UserLevel level) : uid(uid), level(level){};
  };

  struct CachedCards
  {
    std::array<card::uid_t, conf::rfid_tags::CACHE_LEN> cards;
    std::array<FabUser::UserLevel, conf::rfid_tags::CACHE_LEN> levels;

    constexpr CachedCards() : cards{card::INVALID}, levels{FabUser::UserLevel::Unknown} {};

    constexpr const CachedCard operator[](int i) const
    {
      return {cards[i], levels[i]};
    }

    const std::optional<CachedCard> find_uid(const card::uid_t &search_uid) const
    {
      if (search_uid == card::INVALID)
      {
        return std::nullopt;
      }
      const auto pos = std::find(cards.cbegin(), cards.cend(), search_uid);
      if (pos != cards.cend())
      {
        auto idx = std::distance(cards.cbegin(), pos);
        return CachedCard{*pos, levels[idx]};
      }
      return std::nullopt;
    }

    constexpr void set_at(int idx, const card::uid_t &uid, const FabUser::UserLevel &level)
    {
      cards[idx] = uid;
      levels[idx] = level;
    }

    constexpr size_t size() const
    {
      return conf::rfid_tags::CACHE_LEN;
    }
  };

} // namespace fabomatic

#endif // CACHEDCARDS_HPP