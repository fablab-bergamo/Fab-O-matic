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
    void add_in_cache(card::uid_t uid, const std::string &name, FabUser::UserLevel level) const;
    [[nodiscard]] auto is_in_cache(card::uid_t uid) const -> std::optional<FabUser>;
    [[nodiscard]] auto WhiteListLookup(card::uid_t uid) const -> std::optional<WhiteListEntry>;

  public:
    AuthProvider() = delete;
    AuthProvider(WhiteList whitelist);
    [[nodiscard]] auto tryLogin(card::uid_t uid, FabBackend &server) const -> std::optional<FabUser>;
    void setWhitelist(WhiteList list);
    auto saveCache() -> bool;
  };
} // namespace fablabbg
#endif // AUTHPROVIDER_HPP_
