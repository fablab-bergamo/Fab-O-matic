#ifndef CACHEDCARDS_HPP
#define CACHEDCARDS_HPP

#include <array>
#include <optional>

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

    constexpr auto operator[](int i) const -> const CachedCard
    {
      return {cards[i], levels[i]};
    }

    constexpr auto find_uid(const card::uid_t &search_uid) const -> const std::optional<CachedCard>
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

    constexpr auto set_at(int idx, const card::uid_t &uid, const FabUser::UserLevel &level) -> void
    {
      cards[idx] = uid;
      levels[idx] = level;
    }

    constexpr auto size() const -> size_t
    {
      return conf::rfid_tags::CACHE_LEN;
    }
  };

} // namespace fabomatic

#endif // CACHEDCARDS_HPP
