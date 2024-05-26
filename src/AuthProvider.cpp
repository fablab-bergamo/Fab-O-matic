#include "AuthProvider.hpp"

#include <cstdint>
#include <string>
#include <algorithm>

#include "FabBackend.hpp"
#include "Logging.hpp"

namespace fabomatic
{
  namespace Board
  {
    // Only main.cpp instanciates the variables through Board.h file
    extern FabBackend server;
  } // namespace Board

  AuthProvider::AuthProvider(WhiteList list) : whitelist{list}, cache{} {}

  /// @brief Cache the user request
  /// @param uid card id of the user
  /// @param name name of the user to be cached
  /// @param level priviledge level of the user
  constexpr void AuthProvider::updateCache(card::uid_t uid, FabUser::UserLevel level) const
  {
    // Search for the card in the cache
    const auto pos = std::find(cache.cards.cbegin(), cache.cards.cend(), uid);
    if (pos != cache.cards.cend())
    {
      // Update the level at same index
      const auto idx = std::distance(cache.cards.cbegin(), pos);
      cache.levels[idx] = level;
      return;
    }

    // No need to add in cache invalid cards
    if (level == FabUser::UserLevel::Unknown)
      return;

    // Sanity check
    if (cache_idx >= cache.size())
      cache_idx = 0;

    // Add into list
    cache.set_at(cache_idx, uid, level);

    cache_idx = (cache_idx + 1) % conf::rfid_tags::CACHE_LEN;
  }

  /// @brief Verifies the card ID against the server (if available) or the whitelist
  /// @param uid card ID
  /// @param server the server to check the card against
  /// @return a FabUser with an authenticated flag==true if valid, otherwise nullopt or FabUser with authenticated==False.
  auto AuthProvider::tryLogin(card::uid_t uid, FabBackend &server) const -> std::optional<FabUser>
  {
    FabUser user;
    using UserResult = ServerMQTT::UserResult;
    const auto uid_str = card::uid_str(uid);

    ESP_LOGD(TAG, "tryLogin called for %s", uid_str.c_str());

    if (!server.isOnline())
      server.connect();

    if (server.isOnline())
    {
      const auto response = server.checkCard(uid);
      if (response->request_ok)
      {
        user.card_uid = uid;

        if (response->getResult() == UserResult::Authorized)
        {
          user.authenticated = true;
          user.holder_name = response->holder_name;
          user.user_level = response->user_level;
          // Cache the positive result
          updateCache(uid, response->user_level);

          ESP_LOGD(TAG, " -> online check OK (%s)", user.toString().c_str());

          return user;
        }

        // Invalidate the cache entries
        updateCache(uid, FabUser::UserLevel::Unknown);

      } // if (response->request_ok)

      ESP_LOGD(TAG, " -> online check NOT OK");

      user.authenticated = false;

      if (response->request_ok) // Server replied but user is not valid
      {
        return user;
      }
    }
    // Check whitelist if offline
    if (const auto &result = uidInWhitelist(uid); result.has_value())
    {
      const auto &[card, level, name] = result.value();
      user.card_uid = card;
      user.authenticated = true;
      user.user_level = level;
      user.holder_name = name;
      ESP_LOGD(TAG, " -> whilelist check OK (%s)", user.toString().c_str());

      return user;
    }

    // Finally check the cached values
    if (const auto &result = uidInCache(uid); result.has_value())
    {
      const auto &cached = result.value();
      user.card_uid = cached.uid;
      user.authenticated = (cached.level != FabUser::UserLevel::Unknown);
      user.user_level = cached.level;
      user.holder_name = card::uid_str(uid);
      ESP_LOGD(TAG, " -> cache check OK (%s)", user.toString().c_str());
      return user;
    }

    ESP_LOGD(TAG, " -> whilelist check NOK");
    return std::nullopt;
  }

  /// @brief Checks if the card ID is whitelisted
  /// @param uid card ID
  /// @return a whitelistentry object if the card is found in whitelist
  constexpr auto AuthProvider::uidInWhitelist(card::uid_t candidate_uid) const -> std::optional<WhiteListEntry>
  {
    if (candidate_uid == card::INVALID)
    {
      return std::nullopt;
    }

    const auto &elem = std::find_if(whitelist.cbegin(), whitelist.cend(),
                                    [candidate_uid](const auto &input)
                                    {
                                      const auto &[w_uid, w_level, w_name] = input;
                                      return w_uid == candidate_uid;
                                    });

    if (elem == whitelist.cend())
    {
      ESP_LOGD(TAG, "%s not found in whitelist", card::uid_str(candidate_uid).c_str());
      return std::nullopt;
    }

    return {*elem};
  }

  /// @brief Checks if the card ID is whitelisted
  /// @param uid card ID
  /// @return a whitelistentry object if the card is found in whitelist
  constexpr auto AuthProvider::uidInCache(card::uid_t candidate_uid) const -> std::optional<CachedCard>
  {
    return cache.find_uid(candidate_uid);
  }

  /// @brief Loads the cache from EEPROM
  auto AuthProvider::loadCache() -> void
  {
    const auto &config = SavedConfig::LoadFromEEPROM();
    if (!config.has_value())
      return;

    const auto &loaded = config.value().cachedRfid;

    std::copy(loaded.cards.cbegin(), loaded.cards.cend(), cache.cards.begin());
    std::copy(loaded.levels.cbegin(), loaded.levels.cend(), cache.levels.begin());
  }

  /// @brief Sets the whitelist
  /// @param list the whitelist to set
  auto AuthProvider::setWhitelist(WhiteList list) -> void
  {
    whitelist = list;
  }

  /// @brief Saves the cache of RFID to EEPROM
  auto AuthProvider::saveCache() const -> bool
  {
    SavedConfig config = SavedConfig::LoadFromEEPROM().value_or(SavedConfig::DefaultConfig());

    if (std::equal(config.cachedRfid.cards.cbegin(), config.cachedRfid.cards.cend(), cache.cards.cbegin()) &&
        std::equal(config.cachedRfid.levels.cbegin(), config.cachedRfid.levels.cend(), cache.levels.cbegin()))
    {
      ESP_LOGD(TAG, "Cache is the same, not saving");
      return true;
    }

    std::copy(cache.cards.cbegin(), cache.cards.cend(), config.cachedRfid.cards.begin());
    std::copy(cache.levels.cbegin(), cache.levels.cend(), config.cachedRfid.levels.begin());

    return config.SaveToEEPROM();
  }
} // namespace fabomatic