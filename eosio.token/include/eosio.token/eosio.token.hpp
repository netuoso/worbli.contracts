/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosio.token/registered.token.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

// regulated token structs
using registeredtoken::controller;
using registeredtoken::registry;
using registeredtoken::requirement;
using registeredtoken::permission;

namespace eosio {

   using std::string;

   class [[eosio::contract("eosio.token")]] token : public contract {
      public:

         using contract::contract;

         [[eosio::action]]
         void create( name   issuer,
                      asset  maximum_supply);

         [[eosio::action]]
         void issue( name to, asset quantity, string memo );

         [[eosio::action]]
         void retire( asset quantity, string memo );

         [[eosio::action]]
         void transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo );

         [[eosio::action]]
         void open( name owner, const symbol& symbol, name ram_payer );

         [[eosio::action]]
         void close( name owner, const symbol& symbol );

         [[eosio::action]]
         void setcntlr( symbol_code sym, name controller );

         [[eosio::action]]
         void unsetcntlr( symbol_code sym );

         static asset get_supply( name token_contract_account, symbol_code sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

      private:
         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         // regulated token structs
         typedef eosio::multi_index< "controller"_n, controller > controller_table;
         typedef eosio::multi_index< "registries"_n, registry > registry_table;
         typedef eosio::multi_index< "receivereqs"_n, requirement> receivereqs_table;
         typedef eosio::multi_index< "sendreqs"_n, requirement> sendreqs_table;
         typedef eosio::multi_index< "permissions"_n, permission> permission_table;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );
         void is_transfer_allowed( name from, name to, asset quantity );
         void is_receiver_verified( name from, name controller, symbol_code sym);
         void is_sender_verified( name from, name controller, symbol_code sym);
         void verify( name from, sendreqs_table& reqs);
   };

} /// namespace eosio
