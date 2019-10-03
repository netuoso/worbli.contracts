#include <algorithm>

namespace eosiosystem {

   using eosio::time_point;
   using eosio::time_point_sec;
   using eosio::name;

      /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct [[eosio::table, eosio::contract("eosio.system")]] delegated_ram {
      name          from;
      name          to;
      asset         ram_stake;
      int64_t       ram_bytes = 0;

      uint64_t  primary_key()const { return to.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( delegated_ram, (from)(to)(ram_stake)(ram_bytes) )

   };

   struct [[eosio::table, eosio::contract("eosio.system")]] producer_pay {
      name             owner;
      uint64_t         earned_pay;
      uint64_t         last_claim_time = 0;

      uint64_t primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_pay, (owner)(earned_pay)(last_claim_time) )
   };

   struct [[eosio::table("worbliglobal"), eosio::contract("eosio.system")]] worbli_params {
      uint64_t              max_subaccounts = 1;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( worbli_params, (max_subaccounts) )
   };

   struct [[eosio::table("wglobal"), eosio::contract("eosio.system")]] worbli_global_state {
      uint64_t              max_subaccounts = 1;
      time_point_sec        last_inflation_print = time_point_sec();
      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( worbli_global_state, (max_subaccounts)(last_inflation_print) )
   };


   struct [[eosio::table, eosio::contract("eosio.system")]] account_info {
      name                  account;
      name                  parent;
      uint8_t               kyc = 0;
      int64_t               max_subaccounts = -1;
      
      uint64_t primary_key()const { return account.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( account_info, (account)(parent)(kyc)(max_subaccounts) )
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] subaccount {
      name                  account;

      uint64_t primary_key()const { return account.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( subaccount, (account) )
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] feature_toggle {
      name                  feature;
      bool                  is_active;
      uint64_t primary_key()const { return feature.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( feature_toggle, (feature)(is_active) )
   };

   typedef eosio::multi_index< "delram"_n, delegated_ram >        del_ram_table;
   typedef eosio::multi_index< "prodpay"_n, producer_pay >  producer_pay_table;
   typedef eosio::singleton< "worbliglobal"_n, worbli_params >   worbli_params_singleton;
   typedef eosio::singleton< "wglobal"_n, worbli_global_state >   wglobal_state_singleton;
   typedef eosio::multi_index< "accountinfo1"_n, account_info >  account_info_table;
   typedef eosio::multi_index< "subaccounts"_n, subaccount >  subaccount_table;
   typedef eosio::multi_index< "features"_n, feature_toggle >  feature_table;

   // inflation table in worbli.resource contract
   struct daily_inflation
   {
      uint64_t id;
      asset amount;
      eosio::time_point_sec timestamp;
      uint64_t primary_key() const { return (id); }
   };
   typedef eosio::multi_index<"inflation"_n, daily_inflation> inflation_table;

}