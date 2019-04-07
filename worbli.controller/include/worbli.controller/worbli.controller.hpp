#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;       

namespace registeredtoken {

    class [[eosio::contract("worbli.controller")]] worblicontroller : public eosio::contract {
        public:

            using contract::contract; 

            [[eosio::action]]
            void addsendreq( symbol_code sym, name key, name value );

            [[eosio::action]]
            void delsendreq( symbol_code sym, name key, name value );

            [[eosio::action]]
            void addrcvreq( symbol_code sym, name key, name value );

            [[eosio::action]]
            void delrcvreq( symbol_code sym, name key, name value );

            [[eosio::action]]
            void addregistry(symbol_code sym, name registry);

            [[eosio::action]]
            void delregistry(symbol_code sym, name registry);

        private:

            struct currency_stats {
                asset    supply;
                asset    max_supply;
                name     issuer;

                uint64_t primary_key()const { return supply.symbol.code().raw(); }
            };

            // struct for registries grant permissions to onchain accounts 
            struct requirement {
                name     key;
                name     value;

                uint64_t primary_key()const { return key.value; }
            }; 

            // struct for registries grant permissions to onchain accounts 
            struct [[eosio::table("receivereqs")]] receive_requirement : requirement {}; 

            // struct for registries grant permissions to onchain accounts 
            struct [[eosio::table("sendreqs")]] send_requirement : requirement {}; 
            
            struct [[eosio::table]] registries {
                name         registry;

                uint64_t primary_key()const { return registry.value; }
            };

            typedef eosio::multi_index< "stat"_n, currency_stats > stats;

            typedef eosio::multi_index< "registries"_n, registries> registry_table;
            typedef eosio::multi_index< "sendreqs"_n, send_requirement> send_requirements_table;
            typedef eosio::multi_index< "receivereqs"_n, receive_requirement> receive_requirements_table;

    };

} // namespace registeredtoken