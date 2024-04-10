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

  AuthProvider::AuthProvider(WhiteList list) : whitelist{list}, cache{}
  {
    loadCache();
  }

  /// @brief Checks if the cache contains the card ID, and uses that if available
  /// @param uid card id
  /// @return FabUser if found
  auto AuthProvider::is_in_cache(card::uid_t uid) const -> std::optional<FabUser>
  {
    auto elem = std::find_if(cache.begin(), cache.end(),
                             [uid](const auto &input)
                             { return input.card_uid == uid; });

    if (elem == end(cache))
    {
      return std::nullopt;
    }

    return {*elem};
  }

  /// @brief Cache the user request
  /// @param uid card id of the user
  /// @param name name of the user to be cached
  /// @param level priviledge level of the user
  void AuthProvider::add_in_cache(card::uid_t uid, const std::string &name, FabUser::UserLevel level) const
  {
    // Check if already in cache
    if (is_in_cache(uid).has_value())
      return;

    // Add into the list
    FabUser new_elem{uid, name, true, level};
    cache.push_front(new_elem);

    // Keep cache size under CACHE_LEN
    if (cache.size() > conf::rfid_tags::CACHE_LEN)
    {
      cache.pop_back();
    }
  }

  /// @brief Verifies the card ID against the server (if available) or the whitelist
  /// @param uid card ID
  /// @param out a FabUser with an authenticated flag==true if server or whitelist confirmed the ID
  /// @return false if the user was not found on server or whitelist
  auto AuthProvider::tryLogin(card::uid_t uid, FabBackend &server) const -> std::optional<FabUser>
  {
    FabUser user;
    using UserResult = ServerMQTT::UserResult;
    auto uid_str = card::uid_str(uid);

    ESP_LOGD(TAG, "tryLogin called for %s", uid_str.c_str());

    if (!server.isOnline())
      server.connect();

    if (server.isOnline())
    {
      auto response = server.checkCard(uid);
      if (response->request_ok && response->getResult() == UserResult::Authorized)
      {
        user.authenticated = true;
        user.card_uid = uid;
        user.holder_name = response->holder_name;
        user.user_level = response->user_level;
        // Cache the positive result
        add_in_cache(uid, user.holder_name, response->user_level);

        ESP_LOGD(TAG, " -> online check OK (%s)", user.toString().c_str());

        return user;
      }

      ESP_LOGD(TAG, " -> online check NOT OK");

      user.authenticated = false;

      if (response->request_ok)
        return std::nullopt;

      // If request failed, we need to check the whitelist
    }

    if (auto result = WhiteListLookup(uid); result.has_value())
    {
      auto [card, level, name] = result.value();
      user.card_uid = card;
      user.authenticated = true;
      user.user_level = level;
      user.holder_name = name;
      ESP_LOGD(TAG, " -> whilelist check OK (%s)", user.toString().c_str());

      return user;
    }

    ESP_LOGD(TAG, " -> whilelist check NOK");
    return std::nullopt;
  }

  /// @brief Checks if the card ID is whitelisted
  /// @param uid card ID
  /// @return a whitelistentry object if the card is found in whitelist
  auto AuthProvider::WhiteListLookup(card::uid_t candidate_uid) const -> std::optional<WhiteListEntry>
  {
    auto elem = std::find_if(whitelist.begin(), whitelist.end(),
                             [candidate_uid](const auto &input)
                             {
                               auto [w_uid, w_level, w_name] = input;
                               return w_uid == candidate_uid;
                             });

    if (elem == end(whitelist))
    {
      ESP_LOGD(TAG, "%s not found in whitelist", card::uid_str(candidate_uid).c_str());
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

    for (auto &user : config.value().cachedRfid)
    {
      cache.push_back(user);
    }
    // Keep cache size under CACHE_LEN
    while (cache.size() > conf::rfid_tags::CACHE_LEN)
    {
      cache.pop_back();
    }
  }

  /// @brief Sets the whitelist
  /// @param list the whitelist to set
  auto AuthProvider::setWhitelist(WhiteList list) -> void
  {
    whitelist = list;
    cache.clear();
  }

  /// @brief Saves the cache of RFID to EEPROM
  auto AuthProvider::saveCache() -> bool
  {
    SavedConfig config = SavedConfig::LoadFromEEPROM().value_or(SavedConfig::DefaultConfig());
    SavedConfig original{config};

    auto idx = 0;

    // Save the cached value
    for (auto &user : cache)
    {
      config.cachedRfid[idx++] = user;
      if (idx >= conf::rfid_tags::CACHE_LEN)
        break;
    }

    // Clear the rest of the cache
    for (auto i = idx; i < conf::rfid_tags::CACHE_LEN; i++)
    {
      config.cachedRfid[i] = FabUser{};
    }

    // Check if current config is different from updated cachedRfid ignoring order of elements
    if (ScrambledEquals<FabUser, conf::rfid_tags::CACHE_LEN>(original.cachedRfid, config.cachedRfid))
    {
      ESP_LOGD(TAG, "Cache is the same, not saving");
      return true;
    }

    return config.SaveToEEPROM();
  }
} // namespace fablabbg