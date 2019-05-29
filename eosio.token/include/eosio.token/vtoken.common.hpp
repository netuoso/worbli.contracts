#pragma once

#include <eosiolib/singleton.hpp>
#include <eosiolib/multi_index.hpp>

namespace vtoken {

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

   class identity_base {
      public:
         identity_base( account_name acnt) : _self( acnt ) {}

         bool is_trusted_by( account_name trusted, account_name by );

         bool is_trusted( account_name acnt );

      protected:
         account_name _self;
   };

}
