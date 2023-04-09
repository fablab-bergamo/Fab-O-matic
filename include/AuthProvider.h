#ifndef _AUTH_PROVIDER_H_
#define _AUTH_PROVIDER_H_

#include "FabUser.h"
#include <list>

class AuthProvider{
private:
    std::array<card::uid_t, conf::whitelist::LEN> whitelist;
    std::list<FabUser> cache;
    void add_in_cache(card::uid_t uid, std::string name);
    FabUser is_in_cache(card::uid_t uid) const;
    bool isWhiteListed(card::uid_t uid) const;

public:
    AuthProvider(std::array<card::uid_t, conf::whitelist::LEN> whitelist) : whitelist(whitelist) {}
    bool tryLogin(card::uid_t uid, FabUser& out);
};

#endif