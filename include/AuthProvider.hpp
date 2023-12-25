#ifndef AUTHPROVIDER_H_
#define AUTHPROVIDER_H_

#include "FabUser.hpp"
#include <list>
#include <string_view>
#include <tuple>
#include <string>

using WhiteListEntry = std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>;
using WhiteList = std::array<WhiteListEntry, conf::whitelist::LEN>;

class AuthProvider
{
private:
  const WhiteList whitelist;
  mutable std::list<FabUser> cache;
  void add_in_cache(card::uid_t uid, std::string name, FabUser::UserLevel level) const;
  std::optional<FabUser> is_in_cache(card::uid_t uid) const;
  std::optional<WhiteListEntry> WhiteListLookup(card::uid_t uid) const;

public:
  AuthProvider() = delete;
  AuthProvider(WhiteList whitelist);
  [[nodiscard]] std::optional<FabUser> tryLogin(card::uid_t uid) const;
};

#endif // AUTHPROVIDER_H_
