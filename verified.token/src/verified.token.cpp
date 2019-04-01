#include "verified.token/verified.token.hpp"

void verifiedtoken::addregistry(name registry) {
    require_auth( _self );
}

void verifiedtoken::rmvregistry(name registry) {
    require_auth( _self );
}

EOSIO_DISPATCH( verifiedtoken, (addregistry)(rmvregistry) )