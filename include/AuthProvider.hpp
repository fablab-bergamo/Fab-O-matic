#ifndef AUTHPROVIDER_HPP_
#define AUTHPROVIDER_HPP_

#include <list>
#include <string>
#include <string_view>
#include <tuple>

#include "FabUser.hpp"
#include "secrets.hpp"
#include "WhiteList.hpp"

namespace fablabbg
{
  class AuthProvider
  {
  private:
    WhiteList whitelist;
    mutable CachedList cache;
    mutable size_t cache_idx = 0;
    [[nodiscard]] auto uidInWhitelist(card::uid_t uid) const -> std::optional<WhiteListEntry>;
    [[nodiscard]] auto uidInCache(card::uid_t uid) const -> std::optional<CachedFabUser>;
    [[nodiscard]] auto searchCache(card::uid_t candidate_uid) const -> std::optional<CachedFabUser>;
    auto updateCache(card::uid_t candidate_uid, FabUser::UserLevel level) const -> void;

  public:
    AuthProvider() = delete;
    AuthProvider(WhiteList whitelist);
    [[nodiscard]] auto tryLogin(card::uid_t uid, FabBackend &server) const -> std::optional<FabUser>;
    void setWhitelist(WhiteList list);
    auto saveCache() const -> bool;
    auto loadCache() -> void;
  };
} // namespace fablabbg
#endif // AUTHPROVIDER_HPP_
