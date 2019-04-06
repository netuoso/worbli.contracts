#include "worbli.controller/worbli.controller.hpp"

void worblicontroller::addrcvreq( symbol_code sym, name key, name value ) {
    require_auth( _self );

    stats statstable( "eosio.token"_n, sym.raw() );
    auto existing = statstable.find( sym.raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before setting receive requirements" );

    receivereqs receivereqstable(_self, sym.raw());    
    auto recitr = receivereqstable.find( key.value );
    
    if(recitr == receivereqstable.end()) {
        receivereqstable.emplace(_self, [&]( auto& req ) {
            req.key = key;
            req.values.emplace_back(value);
        });
    } else {
        auto valitr = std::find(recitr->values.begin(), recitr->values.end(), value);
        eosio_assert(valitr == recitr->values.end(), "Requirement already exists");
        receivereqstable.modify( *recitr, _self, [&]( auto& req ) {
            req.values.emplace_back(value);
        });
    }
}


void worblicontroller::delrcvreq( symbol_code sym, name key, name value ) {
    require_auth( _self );
    receivereqs receivereqstable(_self, sym.raw());    
    auto recitr = receivereqstable.find( key.value );

    eosio_assert(recitr != receivereqstable.end(), "Requirement does not exist in database");
    auto valitr = std::find(recitr->values.begin(), recitr->values.end(), value);
    eosio_assert(valitr != recitr->values.end(), "Requirement value does not exist in database");
    if(recitr->values.size() > 1) {
        receivereqstable.modify( *recitr, _self, [&]( auto& req ) {
            req.values.erase(valitr);
        });
    } else {
         receivereqstable.erase(recitr);
    }

}

void worblicontroller::addsendreq( name key, name value ) {
    require_auth( _self );
    auto senditr = _sendreqs.find(value.value);
    
    if(senditr == _sendreqs.end()) {
        _sendreqs.emplace(_self, [&]( auto& req ) {
            req.key = key;
            req.values.emplace_back(value);
        });
    } else {
        auto valitr = std::find(senditr->values.begin(), senditr->values.end(), value);
        eosio_assert(valitr == senditr->values.end(), "Requirement already exists");
        _sendreqs.modify( *senditr, _self, [&]( auto& req ) {
            req.values.emplace_back(value);
        });
    }

}

void worblicontroller::delsendreq( name key, name value ) {
    require_auth( _self );
    auto senditr = _sendreqs.find(key.value);
    eosio_assert(senditr != _sendreqs.end(), "Requirement does not exist in database");
    auto valitr = std::find(senditr->values.begin(), senditr->values.end(), value);
    eosio_assert(valitr != senditr->values.end(), "Requirement value does not exist in database");
    if(senditr->values.size() > 1) {
        _sendreqs.modify( *senditr, _self, [&]( auto& req ) {
            req.values.erase(valitr);
        });
    } else {
         _sendreqs.erase(senditr);
    }

}

void worblicontroller::addregistry(name registry) {
    require_auth( _self );
    auto regitr = _registries.find(registry.value);
    eosio_assert( regitr == _registries.end(), "Registry already exists");
    _registries.emplace(_self, [&]( auto& reg ) {
        reg.registry = registry;
    });
}

void worblicontroller::delregistry(name registry) {
    require_auth( _self );
    auto regitr = _registries.find(registry.value);
    eosio_assert( regitr != _registries.end(), "Registry does not exist is database");
    _registries.erase(regitr);
}

EOSIO_DISPATCH( worblicontroller, (addrcvreq)(delrcvreq)(addsendreq)(delsendreq)
                (addregistry)(delregistry) )