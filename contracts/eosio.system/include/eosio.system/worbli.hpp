namespace eosiosystem {

   struct [[eosio::table, eosio::contract("eosio.system")]] account_info {
      name                  account;
      name                  parent;
      bool                  kyc;
      std::vector<name>     children;
      uint64_t              max_children = 0;
      
      uint64_t primary_key()const { return account.value; }

      void validate();

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( account_info, (account)(parent)(kyc)(children)(max_children) )
   };

   typedef eosio::multi_index< "accountinfo"_n, account_info >  account_info_table;

}