namespace eosiosystem {

   struct [[eosio::table, eosio::contract("eosio.system")]] account_info_old {
      name                  account;
      name                  parent;
      bool                  kyc;
      int64_t               max_subaccounts;
      
      uint64_t primary_key()const { return account.value; }

      EOSLIB_SERIALIZE( account_info_old, (account)(parent)(kyc)(max_subaccounts) )
   };

   typedef eosio::multi_index< "accountinfo1"_n, account_info_old >  account_info_table_old;

}