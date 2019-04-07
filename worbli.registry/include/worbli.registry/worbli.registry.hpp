#include <eosiolib/eosio.hpp>

using namespace eosio;

namespace registeredtoken {

    class [[eosio::contract("worbli.registry")]] worbliregistry : public eosio::contract {
        public:
            worbliregistry( name self, name code, datastream<const char*> ds )
            :contract(self, code, ds),
                _permissions(self, self.value)
            {  }

            struct requirement {
                name     key;
                name     value;

                uint64_t primary_key()const { return key.value; }
            }; 

            struct [[eosio::table("persmissions")]] permission {
                uint64_t    pkey;
                name        account;
                requirement requirement;

                uint64_t primary_key()const    { return pkey;             }
                uint64_t by_account()const { return account.value; }
            };

            [[eosio::action]]
            void update(name account, requirement requirement);

            [[eosio::action]]
            void remove(name account, requirement requirement);

        private:

            typedef eosio::multi_index< "permissions"_n, permission,
                                    indexed_by<"account"_n, const_mem_fun<permission, uint64_t, &permission::by_account>  >   
                                    > permission_table;

            permission_table _permissions;
    
    };

} // registeredtoken