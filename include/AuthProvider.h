#ifndef _AUTH_PROVIDER_H_
#define _AUTH_PROVIDER_H_

#include "FabUser.h"
#include <list>
#include <string_view>

class AuthProvider{
private:
    struct LookupResult 
    {
        bool found;
        std::tuple<card::uid_t, FabUser::UserLevel, std::string_view> element;
    };

    std::array<std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>, conf::whitelist::LEN> whitelist;
    std::list<FabUser> cache;
    void add_in_cache(card::uid_t uid, std::string name, FabUser::UserLevel level);
    FabUser is_in_cache(card::uid_t uid) const;
    LookupResult WhiteListLookup(card::uid_t uid) const;

public:
    AuthProvider(std::array<std::tuple<card::uid_t, FabUser::UserLevel, std::string_view>, conf::whitelist::LEN> whitelist) : whitelist(whitelist) {}
    bool tryLogin(card::uid_t uid, FabUser& out);
};

#endif