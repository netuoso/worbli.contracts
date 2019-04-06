#include "worbli.registry/worbli.registry.hpp"

void worbliregistry::update(name account, requirement requirement) {
    require_auth( _self );
}

void worbliregistry::remove(name account, requirement requirement) {
    require_auth( _self );
}

EOSIO_DISPATCH( worbliregistry, (update)(remove) )