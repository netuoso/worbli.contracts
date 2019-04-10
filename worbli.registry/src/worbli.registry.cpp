#include "worbli.registry/worbli.registry.hpp"

namespace registeredtoken {

    void worbliregistry::update(name account, permission permission) {
        require_auth( _self );
        permission_table permissions(_self, account.value);
        auto permitr = permissions.find(permission.key.value);
        if ( permitr == permissions.end() ) {
            permissions.emplace(_self, [&]( auto& perm ) {
                perm.key = permission.key;
                perm.value = permission.value;
            });
        } else {
            permissions.modify( permitr, same_payer, [&]( auto& perm ) {
                perm.value = permission.value;
            });
        }
    }

    void worbliregistry::remove(name account, permission permission) {
        require_auth( _self );
    }

    EOSIO_DISPATCH( worbliregistry, (update)(remove) )

}