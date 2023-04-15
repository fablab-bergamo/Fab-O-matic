#ifndef _AUTH_PROVIDER_H_
#define _AUTH_PROVIDER_H_

#include "FabUser.h"
#include <list>
#include <string_view>

using WhiteListEntry = std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>;
using WhiteList = std::array<WhiteListEntry, conf::whitelist::LEN>;

class AuthProvider
{
private:
    WhiteList whitelist;
    mutable std::list<FabUser> cache;
    void add_in_cache(card::uid_t uid, std::string name, FabUser::UserLevel level) const;
    std::optional<FabUser> is_in_cache(card::uid_t uid) const;
    std::optional<WhiteListEntry> WhiteListLookup(card::uid_t uid) const;

public:
    AuthProvider(WhiteList whitelist) : whitelist(whitelist) {}
    std::optional<FabUser> tryLogin(card::uid_t uid) const;
};

#endif