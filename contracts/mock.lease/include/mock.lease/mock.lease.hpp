#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("mock.lease")]] lease : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void addentry( asset locked );
         
      private:

         struct [[eosio::table]] lease_info
         {
            uint64_t id;
            asset locked;

            uint64_t primary_key() const { return (id); }
         };
         typedef eosio::multi_index<"leasing"_n, lease_info> leasing_table;

   };
   
} /// namespace eosio
