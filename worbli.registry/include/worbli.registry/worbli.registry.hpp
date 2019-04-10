#include <eosiolib/eosio.hpp>

using namespace eosio;

namespace registeredtoken {

    class [[eosio::contract("worbli.registry")]] worbliregistry : public eosio::contract {
        public:

            using contract::contract;

            
            struct [[eosio::table("permissions")]] permission {
                name     key;
                name     value;

                uint64_t primary_key()const { return key.value; }
            }; 

            [[eosio::action]]
            void update(name account, permission permission);

            [[eosio::action]]
            void remove(name account, permission permission);

        private:

            typedef eosio::multi_index< "permissions"_n, permission > permission_table;

    
    };

} // registeredtoken