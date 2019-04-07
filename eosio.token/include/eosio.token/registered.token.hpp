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

    // struct for regitries grant permissions to onchain accounts      
    struct requirement {
        name     key;
        name     value;

        uint64_t primary_key()const { return key.value; }
    }; 

    struct permission {
        uint64_t    pkey;
        name        account;
        requirement requirement;

        uint64_t primary_key()const    { return pkey;             }
        uint64_t by_account()const { return account.value; }
    };

    struct registry {
        name         registry;

        uint64_t primary_key()const { return registry.value; }
    };

    struct [[eosio::table("controller"), eosio::contract("eosio.token")]] controller {
        name     controller;

        uint64_t primary_key()const { return controller.value; }
    };    


} // registered.token