#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

/** struct to define required permissions for interacting with a token
*  two tables will use this struct
*  1. sendrequirmeents: The requirents to send a token
*  2. receiverequirments: The requirements to receive a token
* */

namespace registeredtoken {

    using eosio::name;
    using eosio::symbol;

    struct permission {
        name   key;
        name   value;

        uint64_t primary_key()const { return key.value; }
    };

    
    // struct for regitries grant permissions to onchain accounts      
    struct requirement {
        name                key;
        std::vector<name>   values;

        uint64_t primary_key()const { return key.value; }
    }; 

    struct registry {
        name         registry;

        uint64_t primary_key()const { return registry.value; }
    };

    struct [[eosio::table]] controller {
        symbol   token;
        name     controller;

        uint64_t primary_key()const { return token.code().raw(); }
    };    


} // registered.token