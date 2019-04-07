#include "worbli.controller/worbli.controller.hpp"

namespace registeredtoken {

void worblicontroller::addrcvreq( symbol_code sym, name key, name value ) {
    require_auth( _self );

    stats statstable( "eosio.token"_n, sym.raw() );
    auto existing = statstable.find( sym.raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before setting receive requirements" );

    receive_requirements_table receivereqstable(_self, sym.raw());    
    auto recitr = receivereqstable.find( key.value );

    eosio_assert(recitr == receivereqstable.end(), "Requirement already exists");

    receivereqstable.emplace(_self, [&]( auto& req ) {
        req.key = key;
        req.value = value;
    }); 
}

void worblicontroller::delrcvreq( symbol_code sym, name key, name value ) {
    require_auth( _self );
    receive_requirements_table receivereqstable(_self, sym.raw());    
    auto recitr = receivereqstable.find( key.value );

    eosio_assert(recitr != receivereqstable.end(), "Requirement does not exist in database");
    receivereqstable.erase(recitr);
}

void worblicontroller::addsendreq( symbol_code sym, name key, name value ) {
    require_auth( _self );
    stats statstable( "eosio.token"_n, sym.raw() );
    auto existing = statstable.find( sym.raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before setting receive requirements" );

    send_requirements_table sendreqstable(_self, sym.raw());    
    auto senditr = sendreqstable.find( key.value );

    eosio_assert(senditr == sendreqstable.end(), "Requirement already exists");

    sendreqstable.emplace(_self, [&]( auto& req ) {
        req.key = key;
        req.value = value;
    }); 
}

void worblicontroller::delsendreq( symbol_code sym, name key, name value ) {
    require_auth( _self );

    send_requirements_table sendreqstable(_self, sym.raw());    
    auto senditr = sendreqstable.find( key.value );

    eosio_assert(senditr != sendreqstable.end(), "Requirement does not exist in database");
    sendreqstable.erase(senditr);
}

void worblicontroller::addregistry(symbol_code sym, name registry) {
    require_auth( _self );
    registry_table registrytable(_self, sym.raw()); 
    auto regitr = registrytable.find(registry.value);
    eosio_assert( regitr == registrytable.end(), "Registry already exists");
    registrytable.emplace(_self, [&]( auto& reg ) {
        reg.registry = registry;
    });
}

void worblicontroller::delregistry(symbol_code sym, name registry) {
    require_auth( _self );
    registry_table registries(_self, sym.raw()); 
    auto regitr = registries.find(registry.value);
    eosio_assert( regitr != registries.end(), "Registry does not exist is database");
    registries.erase(regitr);
}

EOSIO_DISPATCH( worblicontroller, (addrcvreq)(delrcvreq)(addsendreq)(delsendreq)
                (addregistry)(delregistry) )

}