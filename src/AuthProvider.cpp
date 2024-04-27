#include "AuthProvider.hpp"

#include <cstdint>
#include <string>

#include "FabBackend.hpp"
#include "Logging.hpp"

namespace fablabbg
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
  void AuthProvider::updateCache(card::uid_t uid, FabUser::UserLevel level) const
  {
    // Search for the card in the cache
    for (auto &cached : cache)
    {
      if (cached.uid == uid)
      {
        cached.level = level;
        return;
      }
    }

    // No need to add in cache invalid cards
    if (level == FabUser::UserLevel::Unknown)
      return;

    // Sanity check
    if (cache_idx >= cache.size())
      cache_idx = 0;

    // Add into list
    cache.at(cache_idx).uid = uid;
    cache.at(cache_idx).level = level;

    cache_idx = (cache_idx + 1) % conf::rfid_tags::CACHE_LEN;
  }

  /// @brief Verifies the card ID against the server (if available) or the whitelist
  /// @param uid card ID
  /// @param out a FabUser with an authenticated flag==true if server or whitelist confirmed the ID
  /// @return false if the user was not found on server or whitelist
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
        if (response->getResult() == UserResult::Authorized)
        {
          user.authenticated = true;
          user.card_uid = uid;
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

      if (response->request_ok)
        return std::nullopt;
    }
    // Check whitelist if offline
    if (auto result = uidInWhitelist(uid); result.has_value())
    {
      const auto [card, level, name] = result.value();
      user.card_uid = card;
      user.authenticated = true;
      user.user_level = level;
      user.holder_name = name;
      ESP_LOGD(TAG, " -> whilelist check OK (%s)", user.toString().c_str());

      return user;
    }

    // Finally check the cached values
    if (auto result = uidInCache(uid); result.has_value())
    {
      const auto cached = result.value();
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
  auto AuthProvider::uidInWhitelist(card::uid_t candidate_uid) const -> std::optional<WhiteListEntry>
  {
    if (candidate_uid == card::INVALID)
    {
      return std::nullopt;
    }

    const auto elem = std::find_if(whitelist.begin(), whitelist.end(),
                             [candidate_uid](const auto &input)
                             {
                               const auto [w_uid, w_level, w_name] = input;
                               return w_uid == candidate_uid;
                             });

    if (elem == end(whitelist))
    {
      ESP_LOGD(TAG, "%s not found in whitelist", card::uid_str(candidate_uid).c_str());
      return std::nullopt;
    }

    return {*elem};
  }

  /// @brief Checks if the card ID is whitelisted
  /// @param uid card ID
  /// @return a whitelistentry object if the card is found in whitelist
  auto AuthProvider::uidInCache(card::uid_t candidate_uid) const -> std::optional<CachedFabUser>
  {
    if (candidate_uid == card::INVALID)
    {
      return std::nullopt;
    }

    const auto elem = std::find_if(cache.begin(), cache.end(),
                             [candidate_uid](const auto &input)
                             {
                               return input.uid == candidate_uid;
                             });

    if (elem == end(cache))
    {
      ESP_LOGD(TAG, "%s not found in cache", card::uid_str(candidate_uid).c_str());
      return std::nullopt;
    }

    return {*elem};
  }

  /// @brief Loads the cache from EEPROM
  auto AuthProvider::loadCache() -> void
  {
    auto config = SavedConfig::LoadFromEEPROM();
    if (!config.has_value())
      return;

    size_t idx = 0;
    for (const auto &user : config.value().cachedRfid)
    {
      cache.at(idx).uid = user.uid;
      cache.at(idx).level = user.level;
      if (user.uid != 0)
      {
        ESP_LOGD(TAG, "Loaded cached RFID tag %s (%d)", card::uid_str(user.uid).c_str(), user.level);
      }
      idx++;
      if (idx >= cache.size())
      {
        idx = 0;
        break;
      }
      if (user.uid != 0)
      {
        cache_idx = idx;
      }
    }
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
    SavedConfig original{config};

    for (auto idx = 0; idx < cache.size(); idx++)
    {
      config.cachedRfid.at(idx).uid = cache.at(idx).uid;
      config.cachedRfid.at(idx).level = cache.at(idx).level;
    }

    // Check if current config is different from updated cachedRfid ignoring order of elements
    if (ScrambledEquals<CachedFabUser, conf::rfid_tags::CACHE_LEN>(original.cachedRfid, config.cachedRfid))
    {
      ESP_LOGD(TAG, "Cache is the same, not saving");
      return true;
    }

    return config.SaveToEEPROM();
  }
} // namespace fablabbg