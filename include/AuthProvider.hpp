#ifndef AUTHPROVIDER_HPP_
#define AUTHPROVIDER_HPP_

#include "FabUser.hpp"
#include "secrets.hpp"
#include <list>
#include <string>
#include <string_view>
#include <tuple>

namespace fablabbg
{
  using WhiteListEntry = std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>;
  using WhiteList = std::array<WhiteListEntry, secrets::cards::LEN>;

  class AuthProvider
  {
  private:
    WhiteList whitelist;
    mutable std::list<FabUser> cache;
    void add_in_cache(card::uid_t uid, const std::string &name, FabUser::UserLevel level) const;
    [[nodiscard]] std::optional<FabUser> is_in_cache(card::uid_t uid) const;
    [[nodiscard]] std::optional<WhiteListEntry> WhiteListLookup(card::uid_t uid) const;

  public:
    AuthProvider() = delete;
    AuthProvider(WhiteList whitelist);
    [[nodiscard]] std::optional<FabUser> tryLogin(card::uid_t uid, FabBackend &server) const;
    void setWhitelist(WhiteList list);
  };
} // namespace fablabbg
#endif // AUTHPROVIDER_HPP_
