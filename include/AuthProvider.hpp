#ifndef AUTHPROVIDER_H_
#define AUTHPROVIDER_H_

#include "FabUser.hpp"
#include <list>
#include <string_view>
#include <tuple>
#include <string>
#include "secrets.hpp"

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
    std::optional<FabUser> is_in_cache(card::uid_t uid) const;
    std::optional<WhiteListEntry> WhiteListLookup(card::uid_t uid) const;

  public:
    AuthProvider() = delete;
    AuthProvider(WhiteList whitelist);
    [[nodiscard]] std::optional<FabUser> tryLogin(card::uid_t uid, FabBackend &server) const;
    void setWhitelist(WhiteList list);
  };
} // namespace fablabbg
#endif // AUTHPROVIDER_H_
