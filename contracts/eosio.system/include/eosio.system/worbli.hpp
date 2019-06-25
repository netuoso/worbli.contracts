#include <algorithm>

namespace eosiosystem {

   struct [[eosio::table("worbliglobal"), eosio::contract("eosio.system")]] worbli_params {
      uint64_t              max_subaccounts = 1;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( worbli_params, (max_subaccounts) )
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

   typedef eosio::singleton< "worbliglobal"_n, worbli_params >   worbli_params_singleton;
   typedef eosio::multi_index< "accountinfo1"_n, account_info >  account_info_table;
   typedef eosio::multi_index< "subaccounts"_n, subaccount >  subaccount_table;

}