namespace eosiosystem {

   struct [[eosio::table, eosio::contract("eosio.system")]] account_info_old {
      name                  account;
      name                  parent;
      bool                  kyc;   
      std::vector<name>     children; 
      
      uint64_t primary_key()const { return account.value; }

      void validate();

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( account_info_old, (account)(parent)(kyc)(children) )
   };

   typedef eosio::multi_index< "accountinfo"_n, account_info_old >  account_info_table_old;

}