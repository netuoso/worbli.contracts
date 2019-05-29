
#include <eosio.token/vtoken.interface.hpp>

namespace vtoken {

    bool interface::allow( account_name acnt, std::vector<pair> requirements ) {
        controller_table controllertable( _self, quantity.symbol.code().raw() );
        auto ctrlitr = controllertable.find( quantity.symbol.code().raw() );
        eosio_assert( ctrlitr != controllertable.end(), "controller for symbol does not exist in database" );
        print("controller: ", ctrlitr->controller);

        receivereqs_table receivereqstable( ctrlitr->controller, quantity.symbol.code().raw() );
        sendreqs_table sendreqstable( ctrlitr->controller, quantity.symbol.code().raw() );
        for( const auto& req : receivereqstable ) {
            print("requirement: ", req.key);
        }

        // lookup registries table in controller scope
        registry_table registries( ctrlitr->controller, quantity.symbol.code().raw() );
        //auto regitr = registrytable.find( quantity.symbol.code().raw() );
        for( const auto& reg : registries ) {
            print("registry: ", reg.registry);
        }
    }

}